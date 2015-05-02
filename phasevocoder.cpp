#include "phasevocoder.h"
#include </usr/local/include/samplerate.h>
#include <QDebug>
#include </usr/local/include/mpg123.h>

/* calculate hamming window for size */
double *hanning_matlab(int N, short itype)
{
    int half, i, idx, n;
    double *w;

    w = (double*) calloc(N, sizeof(double));
    memset(w, 0, N*sizeof(double));

    if(itype==1)    //periodic function
        n = N-1;
    else
        n = N;

    if(n%2==0)
    {
        half = n/2;
        for(i=0; i<half; i++) // Calculates Hanning window samples.
            w[i] = 0.5 * (1 - cos(2*M_PI*(i+1) / (n+1)));

        idx = half-1;
        for(i=half; i<n; i++) {
            w[i] = w[idx];
            idx--;
        }
    }
    else
    {
        half = (n+1)/2;
        for(i=0; i<half; i++) // Calculates Hanning window samples.
            w[i] = 0.5 * (1 - cos(2*M_PI*(i+1) / (n+1)));

        idx = half-2;
        for(i=half; i<n; i++) {
            w[i] = w[idx];
            idx--;
        }
    }

    if(itype==1)    //periodic function
    {
        for(i=N-1; i>=1; i--)
            w[i] = w[i-1];
        w[0] = 0.0;
    }
    return w;
}

PhaseVocoder::PhaseVocoder()
{
    /* set initial variables */
    fft_size = 4096;
    timestretch_ratio = 1.0;
    overlap_factor = 64;
    hop_size = fft_size/overlap_factor;
    an_idx = 1;
    syn_idx = 1;
    tempo_an_idx = 1;
    tempo_syn_idx = 1;
    has_written_until = 0;
    offset = 0;

    /* window to cut back on spectral smearing */
    hanning_window = hanning_matlab(fft_size, 'symmetric');

    /* setup fftw */
    current_window = (double*)fftw_malloc( sizeof(double)*fft_size );
    freq = (double*)fftw_malloc( sizeof(double)*(fft_size) );

    /* fftw plans for forward and inverse transform */
    forward = fftw_plan_r2r_1d(fft_size, current_window, freq, FFTW_R2HC, FFTW_ESTIMATE);
    inverse = fftw_plan_r2r_1d(fft_size, freq, current_window, FFTW_HC2R, FFTW_ESTIMATE);

    /* array for moduli (magnitudes), and phases */
    moduli_idx = (double*)malloc( sizeof(double)*fft_size/2 );
    phases_idx = (double*)malloc( sizeof(double)*fft_size/2 );
    phases_ddx = (double*)malloc( sizeof(double)*fft_size/2 );
    syn_moduli = (double*)malloc( sizeof(double)*fft_size/2 );
    syn_phases_idx = (double*)malloc( sizeof(double)*fft_size/2 );
    syn_phases_ddx = (double*)malloc( sizeof(double)*fft_size/2 );
    phase_increment = (double*)malloc( sizeof(double)*fft_size/2 );

    audio_frames = NULL;
    audio_frames_out = NULL;
}

PhaseVocoder::~PhaseVocoder()
{
    free(moduli_idx);
    free(phases_idx);
    free(phases_ddx);
    free(syn_moduli);
    free(syn_phases_idx);
    free(syn_phases_ddx);
    free(phase_increment);
    free(audio_frames);
    free(audio_frames_out);
    free(current_window);
    free(freq);
    free(forward);
    free(inverse);
}

int read_mp3_file_pv(void* arg)
{
    PhaseVocoder* m = static_cast<PhaseVocoder*>(arg);

    mpg123_handle *mh = NULL;
    float* buffer = NULL;
    QVector<float> vectorBuffer(44100*10);
    size_t buffer_size = 0;
    size_t done = 0;
    int  channels = 0, encoding = 0;
    long rate = 0;
    int  err  = MPG123_OK;
    err = mpg123_init();
    sf_count_t num_frames_read = 0;
    if(err != MPG123_OK || (mh = mpg123_new(NULL, &err)) == NULL)
    {
        qDebug() << "Basic setup goes wrong: " << mpg123_plain_strerror(err);
    }

    mpg123_param(mh, MPG123_ADD_FLAGS, MPG123_FORCE_FLOAT, 0.);
    mpg123_open(mh, m->file_path);
    mpg123_getformat(mh, &rate, &channels, &encoding);
    mpg123_format_none(mh);
    mpg123_format(mh, rate, channels, encoding);
    buffer_size = mpg123_outblock(mh);
    buffer = (float*)malloc( buffer_size );
    do
    {
        err = mpg123_read( mh, (unsigned char*)buffer, buffer_size, &done );
        for(int i = 0; i < buffer_size / sizeof(float); i++) {
            vectorBuffer.append(buffer[i]);
        }
    } while (err == MPG123_OK);

    mpg123_close(mh);
    mpg123_delete(mh);
    mpg123_exit();

    qDebug() << "read mp3 file, had num_frames = " << vectorBuffer.size();
    m->audio_frames = (double*)malloc(vectorBuffer.size()*sizeof(double));
    m->audio_frames_out = (float*)malloc(vectorBuffer.size()*sizeof(float)*2);
    m->audio_frames_reading_from = m->audio_frames;
    for(int i = 0; i < vectorBuffer.size() / 2; ++i) {
        m->audio_frames[i] = (vectorBuffer.at(i*2) + vectorBuffer.at(i*2 + 1)) / 2;
    }

    num_frames_read = vectorBuffer.size() / 2;
    return num_frames_read;
}

void PhaseVocoder::open_audio_file(char* path)
{
    qDebug() << "pv opening audio file";

    file_path = path;

    sf_count_t num_frames_read;

    int path_size = strlen(file_path);
    if(file_path[path_size - 3] == 'm'
            && file_path[path_size - 2] == 'p'
            && file_path[path_size - 1] == '3') {
        qDebug() << "decoding mp3!";
        num_frames_read = read_mp3_file_pv(this);
    }

    else {
        /* open audio file, read into double[] array */
        info.format = SF_FORMAT_WAV;
        SNDFILE* audio_file;
        audio_file = sf_open(path, SFM_READ, &info);

        num_frames = info.frames;

        audio_frames = (double*)malloc(num_frames*info.channels*sizeof(double));
        audio_frames_reading_from = audio_frames;
        audio_frames_out = (float*)malloc(num_frames*info.channels*sizeof(float));
        num_frames_read = sf_readf_double(audio_file, audio_frames, num_frames);
        if (num_frames_read != num_frames)
        {
            qDebug() << "error: unable to read the correct number of frames: read " << num_frames_read << " out of " << num_frames;
        }
        if (info.channels > 1) {
            for(int i = 0; i < num_frames_read; i++) {
                audio_frames[i] = audio_frames[i*info.channels];
            }
        }

        free(audio_file);
    }

    num_frames = num_frames_read;

    /* take windowed sections of the double[] array */
    num_windows = (num_frames / hop_size) - 3; // -3 because the last window takes up 3/4 extra of a hop_size
}

void PhaseVocoder::close_audio_file()
{
    if (audio_frames != NULL)
        free(audio_frames);
    if (audio_frames_out != NULL)
        free(audio_frames_out);
}

void PhaseVocoder::set_timestretch_ratio(double tsr)
{
    this->timestretch_ratio = tsr;
    tempo_an_idx = 1; tempo_syn_idx = 1; // Reset tempo idx's
}

double PhaseVocoder::get_timestretch_ratio()
{
    return this->timestretch_ratio;
}

void PhaseVocoder::fft_routine()
{
    int fuckme = 0;
    while((float)this->tempo_syn_idx / (float)this->tempo_an_idx <= (float)timestretch_ratio)
    {
        /* re-copy phases_idx into phases_ddx */
        memcpy(syn_phases_ddx, syn_phases_idx, fft_size/2*sizeof(double));
        ++fuckme;
        if (fuckme % 4 == 0)
        {
            memset(syn_phases_ddx, 0, fft_size/2*sizeof(double));
        }

        /* populate the phase increase array */
        /* phases for idx */
        memcpy(current_window, &(audio_frames_reading_from[this->an_idx*hop_size]), fft_size*sizeof(double));
        for(int j = 0; j < fft_size; j++)
        {
            current_window[j] *= hanning_window[j];
        }

        fftw_execute(forward);
        for(int i = 0; i < fft_size/2; ++i)
        {
            freq[i] /= 100.0;
            freq[fft_size-i] /= 100.0;
        }

        for(int i = 0; i < fft_size/2; i++)
        {
            moduli_idx[i] = sqrt(fabs(freq[i]*freq[i] + freq[fft_size-i]*freq[fft_size-i])) * 5;
            phases_idx[i] = atan2(freq[fft_size-i], freq[i]);
        }

        /* phases for ddx */
        memcpy(current_window, audio_frames_reading_from+(this->an_idx-1)*hop_size, fft_size*sizeof(double));
        for(int j = 0; j < fft_size; j++)
        {
            current_window[j] *= hanning_window[j];
        }
        fftw_execute(forward);
        for(int i = 0; i < fft_size/2; i++)
        {
            phases_ddx[i] = atan2(freq[fft_size-i], freq[i]);
        }

        /* phase increment */
        for(int i = 0; i < fft_size/2; i++)
        {
            phase_increment[i] = phases_idx[i] - phases_ddx[i];
        }

        peaks = locate_peaks(moduli_idx, fft_size/2);
        int startpoint = 0;
        for(int i = 0; i < (int)peaks.size()-1; i++)
        {
            int peak = peaks.at(i);

            if (this->syn_idx == 1)
            {
                /* copy over the first syn_phases_ddx from phases_ddx */
                memcpy(syn_phases_ddx, phases_ddx, fft_size/2*sizeof(double));
            }

            syn_phases_idx[peak] = syn_phases_ddx[peak] + phase_increment[peak];

            double angle_rotation = syn_phases_idx[peak] - phases_idx[peak];

            int next_peak = peaks.at(i+1);
            int endpoint = ((peak+next_peak)/2);

            for(int ri_indices = startpoint; ri_indices < endpoint+1; ++ri_indices)
            {
                if (ri_indices != peak)
                    syn_phases_idx[ri_indices] = angle_rotation + phases_idx[ri_indices];
            }

            startpoint = endpoint + 1;
        }

        for(int d = 0; d < fft_size/2; ++d)
            syn_moduli[d] = moduli_idx[d];

        /* now have synthesis moduli, phases so just run ifft and do overlap-add */
        for(int f = 0; f < fft_size/2; ++f)
        {
            double real = syn_moduli[f]*cos(syn_phases_idx[f]);
            double imag = syn_moduli[f]*sin(syn_phases_idx[f]);

            freq[f] = real;
            freq[fft_size-f] = imag;
        }

        fftw_execute(inverse);
        for(int f = 0; f < fft_size; ++f)
        {
            audio_frames_out[hop_size*this->syn_idx + f] += current_window[f] * hanning_window[f] / 4;
        }
        this->has_written_until = hop_size*this->syn_idx;

        this->syn_idx += 1;
        this->tempo_syn_idx += 1;
    }
}

void PhaseVocoder::write_output(char* output_path)
{
    printf("writing output... ");

    /* write audio_frames_out buffer to disk */
    SF_INFO out_info;
    out_info.channels = info.channels;
    out_info.format = info.format;
    out_info.frames = info.frames*timestretch_ratio;
    out_info.samplerate = info.samplerate / timestretch_ratio;
    out_info.sections = info.sections;
    out_info.seekable = info.seekable;
    SNDFILE* out_file = sf_open(output_path, SFM_WRITE, &out_info);

    printf("%d\n", num_windows);

    for(int i = 0; i < num_frames; i++)
    {
        audio_frames_out[i] /= 1024;
    }

//    sf_writef_float(out_file, audio_frames_out, num_frames*timestretch_ratio);
    sf_writef_float(out_file, audio_frames_out, 1600000*timestretch_ratio);
    sf_close(out_file);

    printf("done!\n");
}


std::vector<double> locate_peaks(double* magnitudes, int size)
{
    std::vector<double> peaks;

    for(int i = 3; i < size-3; i++)
    {
        if (magnitudes[i] > magnitudes[i-1]
                && magnitudes[i] > magnitudes[i-2]
                && magnitudes[i] > magnitudes[i-3]
                && magnitudes[i] > magnitudes[i+1]
                && magnitudes[i] > magnitudes[i+2]
                && magnitudes[i] > magnitudes[i+3])
        {
            peaks.push_back(i);
        }
    }

    return peaks;
}
