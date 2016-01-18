#include "phasevocoder.h"

double *hanning_matlab(int N); // GALT!!!

float atan2_approx(float y, float x)
{
	//http://pubs.opengroup.org/onlinepubs/009695399/functions/atan2.html
	//Volkan SALMA

	const float ONEQTR_PI = M_PI / 4.0;
	const float THRQTR_PI = 3.0 * M_PI / 4.0;

	float r, angle;
	float abs_y = fabs(y) + 1e-10f;      // kludge to prevent 0/0 condition

	if ( x < 0.0f )
	{
		r = (x + abs_y) / (abs_y - x);
		angle = THRQTR_PI;
	}
	else
	{
		r = (x - abs_y) / (x + abs_y);
		angle = ONEQTR_PI;
	}

	angle += (0.1963f * r * r - 0.9817f) * r;

	if (y < 0.0f)
	{
		return(-angle);     // negate if in quad III or IV
	}
	else
	{
		return(angle);
	}
}

void pv_init(PhaseVocoder* pv)
{
    /* set initial variables */
    pv->fft_size = 4096;
    pv->timestretch_ratio = 1.0;
    pv->overlap_factor = 16;
    pv->hop_size = pv->fft_size/pv->overlap_factor;
    pv->an_idx = 1;
    pv->syn_idx = 1;
    pv->tempo_an_idx = 1;
    pv->tempo_syn_idx = 1;
    pv->has_written_until = 0;
    pv->offset = 0;

    /* window to cut back on spectral smearing */
    pv->hanning_window = hanning_matlab(pv->fft_size);

    /* setup fftw */
    pv->current_window = (double*)fftw_malloc( sizeof(double)*pv->fft_size );
    pv->freq = (double*)fftw_malloc( sizeof(double)*(pv->fft_size) );

    /* fftw plans for forward and inverse transform */
    pv->forward = fftw_plan_r2r_1d(pv->fft_size, pv->current_window, pv->freq, FFTW_R2HC, FFTW_ESTIMATE);
    pv->inverse = fftw_plan_r2r_1d(pv->fft_size, pv->freq, pv->current_window, FFTW_HC2R, FFTW_ESTIMATE);

    /* array for moduli (magnitudes), and phases */
    pv->moduli_idx = (double*)malloc(sizeof(double)*pv->fft_size/2);
    pv->phases_idx = (double*)malloc(sizeof(double)*pv->fft_size/2);
    pv->phases_ddx = (double*)malloc(sizeof(double)*pv->fft_size/2);
    pv->syn_moduli = (double*)malloc(sizeof(double)*pv->fft_size/2);
    pv->syn_phases_idx = (double*)malloc(sizeof(double)*pv->fft_size/2);
    pv->syn_phases_ddx = (double*)malloc(sizeof(double)*pv->fft_size/2);
    pv->phase_increment = (double*)malloc(sizeof(double)*pv->fft_size/2);
		pv->peaks = (double*)malloc(sizeof(double)*pv->fft_size/2);

    pv->audio_frames = NULL;
    pv->audio_frames_out = NULL;
}

void pv_cleanup(PhaseVocoder* pv)
{
    free(pv->moduli_idx);
    free(pv->phases_idx);
    free(pv->phases_ddx);
    free(pv->syn_moduli);
    free(pv->syn_phases_idx);
    free(pv->syn_phases_ddx);
    free(pv->phase_increment);
    free(pv->audio_frames);
    free(pv->audio_frames_out);
    free(pv->current_window);
    free(pv->freq);
    free(pv->forward);
    free(pv->inverse);
		free(pv->peaks);
}

void pv_read_audio_file(PhaseVocoder *pv, char* path)
{
    pv->file_path = path;

    sf_count_t num_frames_read;

		/* open audio file, read into double[] array */
		pv->info.format = SF_FORMAT_WAV;
		SNDFILE* audio_file;
		audio_file = sf_open(path, SFM_READ, &pv->info);

		pv->num_frames = pv->info.frames;

		pv->audio_frames = (double*)malloc(pv->num_frames*pv->info.channels*sizeof(double));
		pv->audio_frames_reading_from = pv->audio_frames;
		pv->audio_frames_out = (float*)malloc(pv->num_frames*pv->info.channels*sizeof(float));
		num_frames_read = sf_readf_double(audio_file, pv->audio_frames, pv->num_frames);
		if (num_frames_read != pv->num_frames)
		{
			printf("error: unable to read the correct number of frames: read %lld out of %lld", num_frames_read, pv->num_frames);
		}
		if (pv->info.channels > 1) 
		{
				for(int i = 0; i < num_frames_read; i++) 
				{
						pv->audio_frames[i] = pv->audio_frames[i*pv->info.channels];
				}
		}

		free(audio_file);

    pv->num_frames = num_frames_read;

    /* take windowed sections of the double[] array */
    pv->num_windows = (pv->num_frames / pv->hop_size) - 3; // -3 because the last window takes up 3/4 extra of a hop_size
}

void pv_fft_routine(PhaseVocoder* pv)
{
    while((float)pv->tempo_syn_idx / (float)pv->tempo_an_idx <= (float)pv->timestretch_ratio)
    {
        /* re-copy phases_idx into phases_ddx */
        memcpy(pv->syn_phases_ddx, pv->syn_phases_idx, pv->fft_size/2*sizeof(double));

        /* populate the phase increase array */
        /* phases for idx */
        memcpy(pv->current_window, 
						&(pv->audio_frames_reading_from[pv->an_idx*pv->hop_size]), 
						pv->fft_size*sizeof(double));
        for(int j = 0; j < pv->fft_size; j++)
        {
            pv->current_window[j] *= pv->hanning_window[j];
        }

        fftw_execute(pv->forward);
        for(int i = 0; i <= pv->fft_size; ++i)
        {
            pv->freq[i] /= 100.0;
        }

        for(int i = 0; i < pv->fft_size/2; i++)
        {
            pv->moduli_idx[i] = sqrt(fabs(pv->freq[i]*pv->freq[i] 
									+ pv->freq[pv->fft_size-i]*pv->freq[pv->fft_size-i])) * 5;
            pv->phases_idx[i] = atan2_approx(pv->freq[pv->fft_size-i], pv->freq[i]);
        }

        /* phases for ddx */
        memcpy(pv->current_window, 
						pv->audio_frames_reading_from+(pv->an_idx-1)*pv->hop_size, 
						pv->fft_size*sizeof(double));

        for(int j = 0; j < pv->fft_size; j++)
        {
            pv->current_window[j] *= pv->hanning_window[j];
        }

        fftw_execute(pv->forward);
        for(int i = 0; i < pv->fft_size/2; i++)
        {
            pv->phases_ddx[i] = atan2_approx(pv->freq[pv->fft_size-i], pv->freq[i]);
        }

        /* phase increment */
        for(int i = 0; i < pv->fft_size/2; i++)
        {
            pv->phase_increment[i] = pv->phases_idx[i] - pv->phases_ddx[i];
        }

				// locate peaks
				int peaks_size = 0;
				for(int i = 3; i < pv->fft_size/2-3; i++)
				{
					if (pv->moduli_idx[i] > pv->moduli_idx[i-1]
							&& pv->moduli_idx[i] > pv->moduli_idx[i-2]
							&& pv->moduli_idx[i] > pv->moduli_idx[i-3]
							&& pv->moduli_idx[i] > pv->moduli_idx[i+1]
							&& pv->moduli_idx[i] > pv->moduli_idx[i+2]
							&& pv->moduli_idx[i] > pv->moduli_idx[i+3])
					{
						pv->peaks[peaks_size] = i;
						peaks_size += 1;
					}
				}

        int startpoint = 0;
        for(int i = 0; i < (int)peaks_size-1; i++)
        {
            int peak = pv->peaks[i];

            if (pv->syn_idx == 1)
            {
                /* copy over the first syn_phases_ddx from phases_ddx */
                memcpy(pv->syn_phases_ddx, 
										pv->phases_ddx, 
										pv->fft_size/2*sizeof(double));
            }

            pv->syn_phases_idx[peak] = pv->syn_phases_ddx[peak] + pv->phase_increment[peak];

            double angle_rotation = pv->syn_phases_idx[peak] - pv->phases_idx[peak];

            int next_peak = pv->peaks[i+1];
            int endpoint = ((peak+next_peak)/2);

            for(int ri_indices = startpoint; ri_indices < endpoint+1; ++ri_indices)
            {
                if (ri_indices != peak)
								{
                    pv->syn_phases_idx[ri_indices] = angle_rotation 
											+ pv->phases_idx[ri_indices];
								}
            }

            startpoint = endpoint + 1;
        }

        for(int d = 0; d < pv->fft_size/2; ++d)
				{
            pv->syn_moduli[d] = pv->moduli_idx[d];
				}

        /* now have synthesis moduli, phases so just run ifft and do overlap-add */
        for(int f = 0; f < pv->fft_size/2; ++f)
        {
            double real = pv->syn_moduli[f]*cos(pv->syn_phases_idx[f]);
            double imag = pv->syn_moduli[f]*sin(pv->syn_phases_idx[f]);

            pv->freq[f] = real;
            pv->freq[pv->fft_size-f] = imag;
        }

        fftw_execute(pv->inverse);
        for(int f = 0; f < pv->fft_size; ++f)
        {
            pv->audio_frames_out[pv->hop_size*pv->syn_idx + f] += 
							pv->current_window[f] * pv->hanning_window[f] / 4;
        }
        pv->has_written_until = pv->hop_size*pv->syn_idx;

        pv->syn_idx += 1;
        pv->tempo_syn_idx += 1;
    }
}

void pv_write_output(PhaseVocoder* pv, char* output_path, int hack)
{
    /* write audio_frames_out buffer to disk */
    SF_INFO out_info;
    out_info.channels = pv->info.channels;
    out_info.format = pv->info.format;
    out_info.frames = pv->info.frames*pv->timestretch_ratio;
    out_info.samplerate = pv->info.samplerate/2;
    out_info.sections = pv->info.sections;
    out_info.seekable = pv->info.seekable;
    SNDFILE* out_file = sf_open(output_path, SFM_WRITE, &out_info);

    for(int i = 0; i < pv->num_frames; i++)
    {
        pv->audio_frames_out[i] /= 2024;
    }

    sf_writef_float(out_file, pv->audio_frames_out, hack * pv->timestretch_ratio);
    sf_close(out_file);
}
