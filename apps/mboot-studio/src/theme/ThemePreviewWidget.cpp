#include "gui/theme/ThemePreviewWidget.hpp"
#include <QPainter>

ThemePreviewWidget::ThemePreviewWidget(QWidget *parent)
    : QWidget(parent)
    , m_themeName("default")
{
    setMinimumSize(200, 150);
}

void ThemePreviewWidget::setThemeName(const QString &name)
{
    m_themeName = name;
    update();
}

void ThemePreviewWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    painter.fillRect(rect(), palette().window());

    painter.setPen(palette().windowText().color());
    QFont f = font();
    f.setPointSize(11);
    painter.setFont(f);
    painter.drawText(rect(), Qt::AlignCenter, tr("Theme: %1").arg(m_themeName));

    int h = height() / 4;
    QRect bar(10, height() - h - 10, width() - 20, h);
    painter.fillRect(bar, palette().highlight());
    painter.setPen(palette().highlightedText().color());
    painter.drawText(bar, Qt::AlignCenter, tr("Preview"));
}
