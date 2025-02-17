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

// spectrumanalyzer.cpp

void SpectrumAnalyzer::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.fillRect(rect(), Qt::black);

    const int barWidth = width() / currentSpectrum.size();
    const int maxHeight = height() - 10;

    // 添加平滑效果
    static QVector<float> lastSpectrum;
    if (lastSpectrum.size() != currentSpectrum.size()) {
        lastSpectrum = currentSpectrum;
    }

    for (int i = 0; i < currentSpectrum.size(); ++i) {
        // 平滑过渡
        lastSpectrum[i] = lastSpectrum[i] * 0.7f + currentSpectrum[i] * 0.3f;
        float value = lastSpectrum[i];

        int barHeight = int(value * maxHeight);

        // 使用HSL颜色空间，基于频率生成颜色
        QColor color;
        if (i < currentSpectrum.size() / 3) {  // 低频 - 红色到黄色
            color = QColor::fromHsl(i * 60 / (currentSpectrum.size() / 3), 255, 128);
        } else if (i < currentSpectrum.size() * 2 / 3) {  // 中频 - 黄色到绿色
            int pos = i - currentSpectrum.size() / 3;
            color = QColor::fromHsl(60 + pos * 60 / (currentSpectrum.size() / 3), 255, 128);
        } else {  // 高频 - 绿色到蓝色
            int pos = i - currentSpectrum.size() * 2 / 3;
            color = QColor::fromHsl(120 + pos * 120 / (currentSpectrum.size() / 3), 255, 128);
        }

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
