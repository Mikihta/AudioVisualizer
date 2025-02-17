#ifndef SPECTRUMANALYZER_H
#define SPECTRUMANALYZER_H

#include <QWidget>
#include <QVector>

class SpectrumAnalyzer : public QWidget
{
    Q_OBJECT

public:
    explicit SpectrumAnalyzer(QWidget *parent = nullptr);

public slots:
    void updateSpectrum(const QVector<float>& spectrum);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QVector<float> currentSpectrum;
    QColor getColor(int index, int total);
};

#endif // SPECTRUMANALYZER_H
