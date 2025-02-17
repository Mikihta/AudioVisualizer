#include "spectrumanalyzer.h"
#include <QPainter>
#include <QLinearGradient>

SpectrumAnalyzer::SpectrumAnalyzer(QWidget *parent)
    : QWidget(parent)
{
    setMinimumSize(400, 200);
    setAttribute(Qt::WA_OpaquePaintEvent);
    currentSpectrum.fill(0, 64);
}

void SpectrumAnalyzer::updateSpectrum(const QVector<float>& spectrum)
{
    currentSpectrum = spectrum;
}

QColor SpectrumAnalyzer::getColor(int index, int total)
{
    float hue = (float)index / total * 360.0f;
    return QColor::fromHsv(hue, 255, 255);
}

void SpectrumAnalyzer::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.fillRect(rect(), Qt::black);

    const int barWidth = width() / currentSpectrum.size();
    const int maxHeight = height() - 10;

    for (int i = 0; i < currentSpectrum.size(); ++i) {
        float value = currentSpectrum[i];
        int barHeight = int(value * maxHeight);

        QColor color = getColor(i, currentSpectrum.size());
        QLinearGradient gradient(
            QPointF(0, height() - barHeight),
            QPointF(0, height()));

        gradient.setColorAt(0, color);
        gradient.setColorAt(1, color.darker());

        painter.fillRect(
            i * barWidth,
            height() - barHeight,
            barWidth - 1,
            barHeight,
            gradient);
    }
}
