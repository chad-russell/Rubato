#include "model.h"
#include <mpg123.h>
#include <QtMultimedia/QAudioFormat>
#include <QtMultimedia/QAudioDecoder>

// Forward declarations
QList<qreal> calculate_cqt(double* audio_frames);
void do_routine(PhaseVocoder* v);
int process(jack_nframes_t nframes, void* arg);

jack_client_t* client;
jack_port_t* jack_output_port;
jack_port_t* jack_output_midi_port;
int global_frame;


Model::Model(QObject *parent) : QObject(parent)
{
    frequencyImageProvider = new FrequencyImageProvider();
    m_percent_done = 0;
    phaseVocoder = new PhaseVocoder();
    playing = false;
    m_percent_loaded = 0;
    m_sample_rate_ratio = 1.0;
    m_freqData = new QVariantList();
    emit percentLoadedChanged();

    /* Set up JACK */
    client = jack_client_open("Rubato", (jack_options_t)0, NULL);
    jack_output_port = jack_port_register(client, "out", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
    jack_output_midi_port = jack_port_register(client, "midi_out", JACK_DEFAULT_MIDI_TYPE, JackPortIsOutput, 0);
    jack_set_process_callback(client, process, this);
    jack_activate(client);
    const char **ports;
    if ((ports = jack_get_ports(client, NULL, NULL, JackPortIsPhysical|JackPortIsInput)) == NULL) {
        qDebug() << "could not find any physical playback ports!";
    }
    for(int i = 0; ports[i] != NULL; ++i) {
        if (jack_connect(client, jack_port_name(jack_output_port), ports[i])) {
            qDebug() << "could not connect to playback port(s)!";
        }
    }

    /* Set up libsamplerate */
    srcState = src_new(SRC_SINC_BEST_QUALITY, 1, NULL);
}

int read_mp3_file(void* arg)
{
    Model* m = static_cast<Model*>(arg);

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
    mpg123_open(mh, m->m_file.path().toUtf8().data());
    mpg123_getformat(mh, &rate, &channels, &encoding);
    mpg123_format_none(mh);
    mpg123_format(mh, rate, channels, encoding);
    buffer_size = mpg123_outblock(mh);
    buffer = (float*)malloc( buffer_size );
    do
    {
        err = mpg123_read( mh, (unsigned char*)buffer, buffer_size, &done );
        for(uint i = 0; i < buffer_size / sizeof(float); i++) {
            vectorBuffer.append(buffer[i]);
        }
    } while (err == MPG123_OK);

    mpg123_close(mh);
    mpg123_delete(mh);
    mpg123_exit();

    qDebug() << "read mp3 file, had num_frames = " << vectorBuffer.size();
    m->m_audio_frames = (double*)malloc(vectorBuffer.size()*sizeof(double));
    for(int i = 0; i < vectorBuffer.size() / 2; ++i) {
        m->m_audio_frames[i] = ((double)vectorBuffer.at(i*2) + (double)vectorBuffer.at(i*2 + 1)) / 2;
    }

    num_frames_read = vectorBuffer.size() / 2;
    return num_frames_read;
}

void read_file(void* arg)
{
    Model* m = static_cast<Model*>(arg);

    global_frame = 0;
    m->m_percent_loaded = 0;
    emit m->percentLoadedChanged();
    sf_count_t num_frames_read;

    if(m->m_file.path().mid(m->m_file.path().length() - 3, 1) == "m"
            && m->m_file.path().mid(m->m_file.path().length() - 2, 1) == "p"
            && m->m_file.path().mid(m->m_file.path().length() - 1, 1) == "3") {
        qDebug() << "decoding mp3!";
        num_frames_read = read_mp3_file(m);
    }
    else {
        SF_INFO info;
        info.format = SF_FORMAT_WAV;
        SNDFILE* audio_file;
        audio_file = sf_open(m->m_file.path().toUtf8().data(), SFM_READ, &info);

        if (audio_file == NULL)
        {
            qDebug() << "error: unable to open file";
            return;
        }

        sf_count_t num_frames = info.frames;
        m->m_audio_frames = (double*)malloc(num_frames * info.channels * sizeof(double));
        num_frames_read = sf_readf_double(audio_file, m->m_audio_frames, num_frames);
        if (info.channels > 1) {
            for(int i = 0; i < num_frames_read; i++) {
                m->m_audio_frames[i] = m->m_audio_frames[i*info.channels];
            }
        }
    }

    QList<qreal> sublist;
    m->m_freqData = new QVariantList();
    int num_cqt_frames = num_frames_read / 2048 - 3;
    for(int f = 0; f < num_cqt_frames; f += 2) {
        m->m_percent_loaded = (qreal)f / num_cqt_frames * 100;
        emit m->percentLoadedChanged();
        sublist = calculate_cqt(m->m_audio_frames + 2048*f);

        m->m_freqData->append(QVariant::fromValue(sublist));
    }

    /* Normalize and Squash (by parts) */
    for(int subpart = 0; subpart < 10; subpart++) {
        // Squash -- powers bring things closer to (or farther from) 1
        for(int i = m->m_freqData->length() * subpart / 10; i < m->m_freqData->length() * (subpart + 1) / 10; i++) {
            QList<qreal> sublist = m->m_freqData->at(i).value< QList<qreal> >();
            for(int j = 0; j < m->m_freqData->at(i).value< QList<qreal> >().length(); j++) {
                sublist.replace(j, pow(sublist.at(j),0.35));
            }
            m->m_freqData->replace(i, QVariant::fromValue(sublist));
        }

        // Now calculate max for normalization
        double max = -10000000;
        for(int i = m->m_freqData->length() * subpart / 10; i < m->m_freqData->length() * (subpart + 1) / 10; i++) {
            for(int j = 0; j < m->m_freqData->at(i).value< QList<qreal> >().length(); j++) {
                if (m->m_freqData->at(i).value< QList<qreal> >().at(j) > max)
                    max = m->m_freqData->at(i).value< QList<qreal> >().at(j);
            }
        }

        // Normalize
        for(int i = m->m_freqData->length() * subpart / 10; i < m->m_freqData->length() * (subpart + 1) / 10; i++) {
            QList<qreal> sublist = m->m_freqData->at(i).value< QList<qreal> >();
            for(int j = 0; j < m->m_freqData->at(i).value< QList<qreal> >().length(); j++) {
                sublist.replace(j, sublist.at(j)*300/max);
            }
            m->m_freqData->replace(i, QVariant::fromValue(sublist));
        }
    }

    m->m_image = QImage(m->m_freqData->length(), 175, QImage::Format_RGB32);
    qDebug() << "fucking length: " << m->m_freqData->length();
    for(int x = 0; x < 4; x++) {
        QList<qreal> t = m->m_freqData->at(x).value< QList<qreal> >();
        for(int y = 0; y < 175; ++y) {
            qreal val = (0.5*t.at(y));
            val = val > 255 ? 255 : val;
//            qreal val = 100;
            m->m_image.setPixel(x, (174 - y),
                                qRgb(val/2,val/1.5,val));
        }
    }
    for(int x = 4; x < m->m_freqData->length(); x++) {
        QList<qreal> t = m->m_freqData->at(x).value< QList<qreal> >();
        QList<qreal> t1 = m->m_freqData->at(x-1).value< QList<qreal> >();
        QList<qreal> t2 = m->m_freqData->at(x-2).value< QList<qreal> >();
        QList<qreal> t3 = m->m_freqData->at(x-3).value< QList<qreal> >();
        for(int y = 0; y < 175; ++y) {
            qreal val = (0.5*t.at(y) + 0.25*t1.at(y) + 0.125*t2.at(y) + 0.125*t3.at(y));
            val = val > 255 ? 255 : val;
//            qreal val = 150;
            m->m_image.setPixel(x, (174 - y),
                                qRgb((int)val/2,(int)val/1.5,(int)val));
//            qDebug() << val;
        }
    }
    m->frequencyImageProvider->image = m->m_image;
    m->frequencyImageProvider->image.save("/home/chad/Desktop/fuckfucfuck.png");

    /* Set up phase vocoder */
    m->phaseVocoder->open_audio_file(m->m_file.path().toUtf8().data());
    m->phaseVocoder->set_timestretch_ratio(1);

    emit m->freqDataChanged();
}

void Model::readFile()
{
    phaseVocoder->close_audio_file(); // Close any open audio files in pv and free memory
    m_freqData = new QVariantList();
    playing = false;
    QtConcurrent::run(read_file, this);
}

void do_routine(PhaseVocoder* v)
{
    /* While we have not calculated far enough, keep going */
    while(v->has_written_until < global_frame + 4096)
    {
        v->fft_routine();
        v->an_idx += 1;
        v->tempo_an_idx += 1;
    }
}

int note_on = -1;
int note_off = -1;
int process(jack_nframes_t nframes, void* arg)
{
    jack_default_audio_sample_t* out = (jack_default_audio_sample_t*)jack_port_get_buffer(jack_output_port, nframes);

    Model* m = (Model*)arg;
    PhaseVocoder* v = m->phaseVocoder;

    if (m->playing) {
        SRC_DATA srcData;
        srcData.data_in = &(v->audio_frames_out[global_frame]);
        srcData.data_out = out;
        srcData.input_frames = nframes * m->m_sample_rate_ratio;
        srcData.output_frames = nframes;
        srcData.src_ratio = 1.0 / m->m_sample_rate_ratio;
        srcData.end_of_input = 0;

        src_process(m->srcState, &srcData);
        for(uint i = 0; i < nframes; i++) {
            out[i] /= 1000;
        }

        for(int i = 0; i < srcData.input_frames; i++) {
            m->setPercentDone(m->m_percent_done + 1.0 / (v->get_timestretch_ratio() * v->num_frames));
        }

        global_frame += srcData.input_frames;
    }
    else {
        for(uint i = 0; i < nframes; i++) {
            out[i] = 0;
        }
    }

    if (note_on == -1 && note_off == -1) {
        void* output_buf = jack_port_get_buffer(jack_output_midi_port, 1);
        jack_midi_clear_buffer(output_buf);
    }
    if (note_on != -1) {
        qDebug() << "note on!";
        note_on = -1;
        void* output_buf = jack_port_get_buffer(jack_output_midi_port, 1);
        jack_midi_clear_buffer(output_buf);
        jack_midi_data_t data[3];
        data[0] = 0x80;
        data[1] = 0x4d;
        data[2] = 0x40;
        jack_midi_event_write(output_buf, 10, data, 3);
    }
    if (note_off != -1) {
        qDebug() << "note off!";
        note_off = -1;
        void* output_buf = jack_port_get_buffer(jack_output_midi_port, 1);
        jack_midi_clear_buffer(output_buf);
        jack_midi_data_t data[3];
        data[0] = 0x80;
        data[1] = 0x4d;
        data[2] = 0x40;
        jack_midi_event_write(output_buf, 10, data, 3);
    }

    if (m->playing && m->percentDone() > 0.999) {
        m->setPlaying(false);
    }
    else if (m->playing) {
        std::thread th(do_routine, v);
        th.detach();
    }

    return 0;
}

void Model::play()
{
    if (!playing) {
        qDebug() << "doing routine";
        do_routine(phaseVocoder);
    }
    playing = !playing;
    emit playingChanged();
}

void Model::seekTo(qreal pct)
{
    for(int i = 0; i < phaseVocoder->num_frames; i++) {
        phaseVocoder->audio_frames_out[i] = 0;
    }
    phaseVocoder->an_idx = 1;
    phaseVocoder->syn_idx = 1;
    phaseVocoder->tempo_an_idx = 1;
    phaseVocoder->tempo_syn_idx = 1;
    phaseVocoder->has_written_until = 0;
    phaseVocoder->audio_frames_reading_from = &(phaseVocoder->audio_frames[(int)(pct * phaseVocoder->num_frames)]);
    offset = (int)(pct * phaseVocoder->num_frames);
    global_frame = 0;
    m_percent_done = (qreal)(pct * phaseVocoder->num_frames) / (qreal)phaseVocoder->num_frames;
}

void Model::setTimestretchRatio(double tr)
{
    if (phaseVocoder != NULL) {
        qDebug() << "got request to set tsr to " << tr;
        phaseVocoder->set_timestretch_ratio(tr);
    }
}

void Model::noteOn(int midi_note_number)
{
    qDebug() << "note on: " << midi_note_number;
    note_on = midi_note_number;
}

void Model::noteOff(int midi_note_number)
{
    qDebug() << "note off: " << midi_note_number;
    note_off = midi_note_number;
}

Model::~Model()
{
    qDebug() << "destructor called";
    free(phaseVocoder);
    free(m_audio_frames);
    free(frequencyImageProvider);
    src_delete(srcState);
}
