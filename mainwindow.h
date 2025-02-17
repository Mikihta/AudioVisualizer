#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include "audiocapture.h"
#include "spectrumanalyzer.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    Ui::MainWindow *ui;
    AudioCapture *audioCapture;
    SpectrumAnalyzer *spectrumAnalyzer;
    QTimer updateTimer;
};

#endif // MAINWINDOW_H
