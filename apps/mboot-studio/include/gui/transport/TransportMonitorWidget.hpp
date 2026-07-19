#pragma once

#include <QWidget>
#include <QTabWidget>
#include <memory>

class QLabel;
class QPushButton;

class TransportMonitorWidget : public QWidget {
    Q_OBJECT
public:
    explicit TransportMonitorWidget(QWidget *parent = nullptr);
    void setUsbStats(const QVariantMap &stats);
    void setSerialStats(const QVariantMap &stats);
    void setTcpStats(const QVariantMap &stats);
    void resetStats();

private:
    void setupUi();
    QTabWidget *m_tabs;
    QLabel *m_latencyLabel, *m_bandwidthLabel, *m_packetsLabel;
    QLabel *m_errorsLabel, *m_reconnectLabel;
    QPushButton *m_resetBtn;
};
