#ifndef AUDIOCAPTURE_H
#define AUDIOCAPTURE_H

#include <QThread>
#include <QVector>
#include <windows.h>
#include <mmdeviceapi.h>
#include <Audioclient.h>

class AudioCapture : public QThread
{
    Q_OBJECT

public:
    AudioCapture(QObject *parent = nullptr);
    ~AudioCapture();

    void stop();

signals:
    void audioDataReady(const QVector<float>& spectrum);

protected:
    void run() override;

private:
    bool running;
    void captureAudio();
    QVector<float> processAudioData(const QVector<float>& rawData);
};

#endif // AUDIOCAPTURE_H
