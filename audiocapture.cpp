#include "audiocapture.h"
#include <QDebug>
#include <complex>
#include <cmath>

AudioCapture::AudioCapture(QObject *parent)
    : QThread(parent), running(true)
{
}

AudioCapture::~AudioCapture()
{
    stop();
    wait();
}

void AudioCapture::stop()
{
    running = false;
}

void AudioCapture::run()
{
    CoInitialize(nullptr);

    IMMDeviceEnumerator *enumerator = nullptr;
    HRESULT hr = CoCreateInstance(
        __uuidof(MMDeviceEnumerator), nullptr,
        CLSCTX_ALL, __uuidof(IMMDeviceEnumerator),
        (void**)&enumerator);

    if (FAILED(hr)) {
        qDebug() << "Failed to create device enumerator";
        return;
    }

    IMMDevice *device = nullptr;
    hr = enumerator->GetDefaultAudioEndpoint(
        eRender, eConsole, &device);

    if (FAILED(hr)) {
        enumerator->Release();
        qDebug() << "Failed to get default audio endpoint";
        return;
    }

    IAudioClient *audioClient = nullptr;
    hr = device->Activate(
        __uuidof(IAudioClient), CLSCTX_ALL,
        nullptr, (void**)&audioClient);

    if (FAILED(hr)) {
        device->Release();
        enumerator->Release();
        qDebug() << "Failed to activate audio client";
        return;
    }

    WAVEFORMATEX *format = nullptr;
    hr = audioClient->GetMixFormat(&format);

    if (FAILED(hr)) {
        audioClient->Release();
        device->Release();
        enumerator->Release();
        qDebug() << "Failed to get mix format";
        return;
    }

    hr = audioClient->Initialize(
        AUDCLNT_SHAREMODE_SHARED,
        AUDCLNT_STREAMFLAGS_LOOPBACK,
        0, 0, format, nullptr);

    if (FAILED(hr)) {
        CoTaskMemFree(format);
        audioClient->Release();
        device->Release();
        enumerator->Release();
        qDebug() << "Failed to initialize audio client";
        return;
    }

    IAudioCaptureClient *captureClient = nullptr;
    hr = audioClient->GetService(
        __uuidof(IAudioCaptureClient),
        (void**)&captureClient);

    if (FAILED(hr)) {
        CoTaskMemFree(format);
        audioClient->Release();
        device->Release();
        enumerator->Release();
        qDebug() << "Failed to get capture client";
        return;
    }

    hr = audioClient->Start();
    if (FAILED(hr)) {
        captureClient->Release();
        CoTaskMemFree(format);
        audioClient->Release();
        device->Release();
        enumerator->Release();
        qDebug() << "Failed to start audio client";
        return;
    }

    while (running) {
        UINT32 packetLength = 0;
        BYTE *data;
        DWORD flags;

        hr = captureClient->GetNextPacketSize(&packetLength);
        if (FAILED(hr)) {
            break;
        }

        if (packetLength > 0) {
            hr = captureClient->GetBuffer(
                &data,
                &packetLength,
                &flags,
                nullptr,
                nullptr);

            if (SUCCEEDED(hr)) {
                QVector<float> audioData;
                float *floatData = (float*)data;
                for (UINT32 i = 0; i < packetLength; i++) {
                    audioData.append(floatData[i]);
                }

                QVector<float> spectrum = processAudioData(audioData);
                emit audioDataReady(spectrum);

                captureClient->ReleaseBuffer(packetLength);
            }
        }

        Sleep(1);
    }

    audioClient->Stop();
    captureClient->Release();
    CoTaskMemFree(format);
    audioClient->Release();
    device->Release();
    enumerator->Release();
    CoUninitialize();
}


QVector<float> AudioCapture::processAudioData(const QVector<float>& rawData)
{
    QVector<float> spectrum(SPECTRUM_SIZE, 0.0f);

    // 准备FFT数据
    std::vector<std::complex<float>> fftData(FFT_SIZE);

    // 填充FFT数据并应用汉宁窗
    for (int i = 0; i < FFT_SIZE && i < rawData.size(); ++i) {
        float hanningWindow = 0.5f * (1.0f - cos(2.0f * M_PI * i / (FFT_SIZE - 1)));
        fftData[i] = std::complex<float>(rawData[i] * hanningWindow, 0.0f);
    }

    // 执行FFT
    performFFT(fftData);

    // 频率分段设置（每个频段的起始和结束频率）
    const int frequencyBands[SPECTRUM_SIZE + 1] = {
        20, 25, 32, 40, 50, 63, 80, 100, 125, 160,
        200, 250, 315, 400, 500, 630, 800, 1000, 1250, 1600,
        2000, 2500, 3150, 4000, 5000, 6300, 8000, 10000, 12500, 16000,
        20000 // 最后一个值作为最高频率界限
    };

    // 计算每个频段的能量
    for (int i = 0; i < SPECTRUM_SIZE; ++i) {
        int startFreq = frequencyBands[i];
        int endFreq = frequencyBands[i + 1];

        float magnitude = getFrequencyMagnitude(fftData, startFreq, endFreq);

        // 对数映射以增强低频显示效果
        magnitude = std::log10(1 + magnitude) * 0.5f;

        // 限制在0-1范围内
        spectrum[i] = std::min(1.0f, std::max(0.0f, magnitude));
    }

    return spectrum;
}

void AudioCapture::performFFT(std::vector<std::complex<float>>& data)
{
    const int n = data.size();
    if (n <= 1) return;

    // 分割数组
    std::vector<std::complex<float>> even(n/2);
    std::vector<std::complex<float>> odd(n/2);
    for (int i = 0; i < n/2; i++) {
        even[i] = data[2*i];
        odd[i] = data[2*i+1];
    }

    // 递归计算
    performFFT(even);
    performFFT(odd);

    // 合并结果
    for (int k = 0; k < n/2; k++) {
        float angle = -2 * M_PI * k / n;
        std::complex<float> t = std::polar(1.0f, angle) * odd[k];
        data[k] = even[k] + t;
        data[k + n/2] = even[k] - t;
    }
}

float AudioCapture::getFrequencyMagnitude(const std::vector<std::complex<float>>& fftData,
                                          int startFreq, int endFreq)
{
    float totalMagnitude = 0.0f;
    int startBin = startFreq * FFT_SIZE / SAMPLE_RATE;
    int endBin = endFreq * FFT_SIZE / SAMPLE_RATE;

    // 确保bin索引在有效范围内
    startBin = std::max(0, std::min(startBin, (int)fftData.size()/2));
    endBin = std::max(0, std::min(endBin, (int)fftData.size()/2));

    int binCount = 0;
    for (int bin = startBin; bin < endBin; ++bin) {
        float magnitude = std::abs(fftData[bin]);
        totalMagnitude += magnitude;
        ++binCount;
    }

    return binCount > 0 ? totalMagnitude / binCount : 0.0f;
}
