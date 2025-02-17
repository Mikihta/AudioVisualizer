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
    const int SPECTRUM_SIZE = 64;
    QVector<float> spectrum(SPECTRUM_SIZE, 0.0f);

    // Simple FFT-like processing
    for (int i = 0; i < SPECTRUM_SIZE && i < rawData.size(); i++) {
        spectrum[i] = std::abs(rawData[i]);
    }

    return spectrum;
}
