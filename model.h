#ifndef MODEL_H
#define MODEL_H

#include <QObject>
#include <QDebug>
#include <QUrl>
#include <QList>
#include <QImage>
#include "frequencyimageprovider.h"
#include "phasevocoder.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <thread>
#include <jack/jack.h>
#include <jack/midiport.h>
#include <samplerate.h>
#include <qmath.h>
#include <QDebug>
#include <QUrl>
#include <QVariantList>
#include <QtConcurrent/QtConcurrent>
#include <QtMultimedia>

class Model : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QUrl file READ file WRITE setFile NOTIFY fileChanged)
    Q_PROPERTY(QImage freqData READ freqData NOTIFY freqDataChanged)
    Q_PROPERTY(qreal percentDone READ percentDone WRITE setPercentDone NOTIFY percentDoneChanged)
    Q_PROPERTY(bool playing READ isPlaying WRITE setPlaying NOTIFY playingChanged)
    Q_PROPERTY(qreal percentLoaded READ percentLoaded NOTIFY percentLoadedChanged)

public:
    explicit Model(QObject *parent = 0);
    ~Model();

    QUrl file() { return m_file; }
    void setFile(QUrl fn) { m_file = fn; readFile(); emit fileChanged(); }
    void readFile();
    qreal percentDone() { return m_percent_done; }
    void setPercentDone(qreal pd) { m_percent_done = pd; emit percentDoneChanged(); }
    Q_INVOKABLE void emitPercentDoneChanged() { emit percentDoneChanged(); }
    Q_INVOKABLE void noteOn(int midi_note_number);
    Q_INVOKABLE void noteOff(int midi_note_number);
    bool playing;
    bool isPlaying() { return playing; }
    void setPlaying(bool p) { playing = p; emit playingChanged(); }
    int offset;
    qreal m_percent_done;
    qreal percentLoaded() { return m_percent_loaded; }

    QImage freqData() { return m_image; }

    FrequencyImageProvider *frequencyImageProvider;
    Q_INVOKABLE void play();
    Q_INVOKABLE void seekTo(qreal pct);
    Q_INVOKABLE void setTimestretchRatio(double tr);
    Q_INVOKABLE void setSampleRateRatio(qreal srr) { m_sample_rate_ratio = srr; }

    PhaseVocoder* phaseVocoder;
    QUrl m_file;
    QVariantList *m_freqData;
    QImage m_image;
    double* m_audio_frames;
    qreal m_percent_loaded;
    SRC_STATE* srcState;
    qreal m_sample_rate_ratio;
    QAudioDecoder* decoder;


private:

signals:
    void fileChanged();
    void freqDataChanged();
    void percentDoneChanged();
    void playingChanged();
    void percentLoadedChanged();

public slots:

};

#endif // MODEL_H
