#include "gui/framework/SplashScreen.hpp"
#include <QPainter>
#include <QApplication>

SplashScreen::SplashScreen()
    : QSplashScreen()
{
    QPixmap pix(600, 400);
    pix.fill(QColor(30, 30, 30));
    setPixmap(pix);
    QFont f;
    f.setPointSize(24);
    f.setBold(true);
    setFont(f);
    showStatus("Loading...");
}

SplashScreen::~SplashScreen() = default;

void SplashScreen::showStatus(const QString &message)
{
    showMessage(message, Qt::AlignBottom | Qt::AlignHCenter, QColor(0, 170, 255));
    QApplication::processEvents();
    emit statusChanged(message);
}

void SplashScreen::setProgress(int percent)
{
    m_progress = qBound(0, percent, 100);
    emit progressChanged(m_progress);
}

void SplashScreen::finishWithDelay(QWidget *mainWindow, int delayMs)
{
    m_timer.singleShot(delayMs, this, [this, mainWindow]() {
        finish(mainWindow);
        emit finished();
    });
}
