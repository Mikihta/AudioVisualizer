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

    // 使用更均匀的频率分布
    const float minFreq = 20.0f;
    const float maxFreq = 20000.0f;

    for (int i = 0; i < SPECTRUM_SIZE; ++i) {
        // 使用指数分布来计算频率范围
        float startFreq = minFreq * std::pow(maxFreq/minFreq, (float)i / SPECTRUM_SIZE);
        float endFreq = minFreq * std::pow(maxFreq/minFreq, (float)(i + 1) / SPECTRUM_SIZE);

        float magnitude = getFrequencyMagnitude(fftData, startFreq, endFreq);

        // 应用对数映射和归一化
        magnitude = std::log10(1 + magnitude * 100) / 2.0f;

        // 添加一个小的基础值，确保始终有微小的显示
        spectrum[i] = std::min(1.0f, std::max(0.1f, magnitude));
    }

    // 应用平滑处理
    static QVector<float> lastSpectrum = spectrum;
    for (int i = 0; i < SPECTRUM_SIZE; ++i) {
        spectrum[i] = lastSpectrum[i] * 0.7f + spectrum[i] * 0.3f;
    }
    lastSpectrum = spectrum;

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
                                          float startFreq, float endFreq)
{
    float totalMagnitude = 0.0f;

    // 将频率转换为FFT bin索引
    int startBin = std::max(1, (int)(startFreq * FFT_SIZE / SAMPLE_RATE));
    int endBin = std::min((int)(endFreq * FFT_SIZE / SAMPLE_RATE), FFT_SIZE/2);

    // 确保至少有一个bin
    endBin = std::max(startBin + 1, endBin);

    float binCount = 0;
    for (int bin = startBin; bin < endBin; ++bin) {
        // 计算bin的幅度，并应用频率权重
        float magnitude = std::abs(fftData[bin]);
        float frequency = bin * SAMPLE_RATE / FFT_SIZE;

        // 对高频进行轻微提升
        float frequencyWeight = std::pow(frequency / startFreq, 0.2f);
        totalMagnitude += magnitude * frequencyWeight;
        binCount += 1.0f;
    }

    return binCount > 0 ? totalMagnitude / binCount : 0.0f;
}

