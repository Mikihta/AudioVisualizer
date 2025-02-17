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
    static const int FFT_SIZE = 4096;       // 增加FFT大小以提高频率分辨率
    static const int SPECTRUM_SIZE = 64;     // 频谱柱状图数量
    static const int SAMPLE_RATE = 44100;    // 采样率

    bool running;
    void captureAudio();
    QVector<float> processAudioData(const QVector<float>& rawData);
    void performFFT(std::vector<std::complex<float>>& data);

    // 更新了参数类型，从 int 改为 float
    float getFrequencyMagnitude(const std::vector<std::complex<float>>& fftData,
                                float startFreq, float endFreq);
};

#endif // AUDIOCAPTURE_H
