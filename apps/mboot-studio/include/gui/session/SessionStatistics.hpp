#pragma once

#include <QWidget>
#include <QLabel>
#include <memory>

class SessionStatistics : public QWidget {
    Q_OBJECT
public:
    explicit SessionStatistics(QWidget *parent = nullptr);
    void setBytesRead(qint64 bytes);
    void setBytesWritten(qint64 bytes);
    void setTransferSpeed(double bps);
    void setElapsed(qint64 ms);
    void setErrors(int count);
    void setWarnings(int count);
    void reset();

private:
    void setupUi();
    QLabel *m_bytesReadLabel, *m_bytesWrittenLabel;
    QLabel *m_speedLabel, *m_elapsedLabel;
    QLabel *m_errorsLabel, *m_warningsLabel;
};
