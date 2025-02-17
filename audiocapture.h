#ifndef AUDIOCAPTURE_H
#define AUDIOCAPTURE_H

#include <QThread>
#include <QVector>
#include <windows.h>
#include <mmdeviceapi.h>
#include <Audioclient.h>
#include <complex>
#include <vector>

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
    static const int FFT_SIZE = 2048;        // FFT 采样点数
    static const int SPECTRUM_SIZE = 64;      // 最终显示的频段数
    static const int SAMPLE_RATE = 44100;     // 采样率

    bool running;
    void captureAudio();
    QVector<float> processAudioData(const QVector<float>& rawData);
    void performFFT(std::vector<std::complex<float>>& data);
    float getFrequencyMagnitude(const std::vector<std::complex<float>>& fftData,
                                int startFreq, int endFreq);
};

#endif // AUDIOCAPTURE_H
