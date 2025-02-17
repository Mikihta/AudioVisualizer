#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    setWindowTitle("Audio Visualizer");

    // 不再需要手动创建SpectrumAnalyzer，因为已经在UI中定义
    spectrumAnalyzer = ui->visualizer;

    audioCapture = new AudioCapture();
    audioCapture->start();

    connect(audioCapture, &AudioCapture::audioDataReady,
            spectrumAnalyzer, &SpectrumAnalyzer::updateSpectrum);

    updateTimer.setInterval(30);
    updateTimer.start();

    connect(&updateTimer, &QTimer::timeout,
            spectrumAnalyzer, QOverload<>::of(&SpectrumAnalyzer::update));
}

MainWindow::~MainWindow()
{
    audioCapture->stop();
    delete audioCapture;
    delete ui;
}
