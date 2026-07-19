#pragma once

#include <QSplashScreen>
#include <QString>
#include <QTimer>

class SplashScreen : public QSplashScreen {
    Q_OBJECT
public:
    explicit SplashScreen();
    ~SplashScreen() override;

    void showStatus(const QString &message);
    void setProgress(int percent);
    void finishWithDelay(QWidget *mainWindow, int delayMs = 500);

signals:
    void progressChanged(int percent);
    void statusChanged(const QString &message);
    void finished();

private:
    QTimer m_timer;
    int m_progress{0};
};
