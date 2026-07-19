#pragma once

#include <QWidget>
#include <QLabel>
#include <memory>

class FlashStatistics : public QWidget {
    Q_OBJECT
public:
    explicit FlashStatistics(QWidget *parent = nullptr);
    void setBytesTransferred(qint64 bytes);
    void setTransferSpeed(double bps);
    void setRetryCount(int count);
    void setErrorCount(int count);
    void setWarningCount(int count);
    void setElapsed(qint64 ms);
    void reset();

private:
    void setupUi();
    QLabel *m_bytesLabel, *m_speedLabel, *m_retryLabel;
    QLabel *m_errorsLabel, *m_warningsLabel, *m_elapsedLabel;
};
