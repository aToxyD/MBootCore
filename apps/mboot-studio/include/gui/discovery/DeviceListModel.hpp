#pragma once

#include <QAbstractListModel>
#include <QList>
#include <QString>
#include <QIcon>

struct DeviceInfo {
    QString id;
    QString name;
    QString vendor;
    QString protocol;
    QString transport;
    QString status;
    QString bootMode;
    int confidence{0};
    bool connected{false};
};

class DeviceListModel : public QAbstractListModel {
    Q_OBJECT
public:
    enum Roles {
        IdRole = Qt::UserRole + 1,
        NameRole,
        VendorRole,
        ProtocolRole,
        TransportRole,
        StatusRole,
        BootModeRole,
        ConfidenceRole,
        ConnectedRole,
        IconRole
    };

    explicit DeviceListModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    void setDevices(const QList<DeviceInfo> &devices);
    void addDevice(const DeviceInfo &device);
    void removeDevice(const QString &id);
    void updateDevice(const DeviceInfo &device);
    void clear();
    DeviceInfo deviceAt(int row) const;
    DeviceInfo deviceById(const QString &id) const;
    int count() const { return static_cast<int>(m_devices.size()); }

signals:
    void devicesChanged();

private:
    QList<DeviceInfo> m_devices;
};
