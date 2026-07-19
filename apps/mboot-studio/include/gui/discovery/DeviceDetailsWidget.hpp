#pragma once

#include <QWidget>
#include <QLabel>
#include <memory>

#include "DeviceListModel.hpp"

class QPushButton;

class DeviceDetailsWidget : public QWidget {
    Q_OBJECT
public:
    explicit DeviceDetailsWidget(QWidget *parent = nullptr);

    void showDevice(const DeviceInfo &device);
    void clear();

signals:
    void connectRequested(const QString &deviceId);
    void disconnectRequested(const QString &deviceId);

private:
    void setupUi();

    QLabel *m_nameLabel;
    QLabel *m_vendorLabel;
    QLabel *m_protocolLabel;
    QLabel *m_transportLabel;
    QLabel *m_statusLabel;
    QLabel *m_bootModeLabel;
    QLabel *m_confidenceLabel;
    QLabel *m_iconLabel;
    QPushButton *m_connectBtn;
    QPushButton *m_disconnectBtn;
    QString m_currentDeviceId;
};
