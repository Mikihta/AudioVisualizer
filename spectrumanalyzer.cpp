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
    painter.setRenderHint(QPainter::Antialiasing);
    painter.fillRect(rect(), Qt::black);

    if (currentSpectrum.isEmpty())
        return;

    const int barWidth = width() / currentSpectrum.size();
    const int maxHeight = height() - 10;

    // 存储上一帧的数据用于平滑处理
    static QVector<float> lastHeights(currentSpectrum.size(), 0);

    for (int i = 0; i < currentSpectrum.size(); ++i) {
        // 平滑过渡
        float targetHeight = currentSpectrum[i] * maxHeight;
        float& lastHeight = lastHeights[i];
        lastHeight = lastHeight * 0.8f + targetHeight * 0.2f;

        int barHeight = static_cast<int>(lastHeight);

        // 确保最小高度
        barHeight = std::max(2, barHeight);

        // 计算颜色
        float hue = i * 360.0f / currentSpectrum.size();
        QColor baseColor = QColor::fromHsv(hue, 255, 255);

        // 创建渐变
        QLinearGradient gradient(
            QPointF(0, height() - barHeight),
            QPointF(0, height()));

        gradient.setColorAt(0, baseColor);
        gradient.setColorAt(1, baseColor.darker(200));

        // 绘制主条
        painter.fillRect(
            i * barWidth + 1,  // 添加1像素间距
            height() - barHeight,
            barWidth - 2,      // 减少2像素宽度形成间距
            barHeight,
            gradient);

        // 绘制顶部高光
        painter.fillRect(
            i * barWidth + 1,
            height() - barHeight,
            barWidth - 2,
            2,
            baseColor.lighter(150));
    }
}
