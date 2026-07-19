#include "gui/discovery/DeviceListModel.hpp"

DeviceListModel::DeviceListModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int DeviceListModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : static_cast<int>(m_devices.size());
}

QVariant DeviceListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_devices.size())
        return {};

    const auto &dev = m_devices.at(index.row());

    switch (role) {
    case Qt::DisplayRole:
    case NameRole:
        return dev.name;
    case IdRole:
        return dev.id;
    case VendorRole:
        return dev.vendor;
    case ProtocolRole:
        return dev.protocol;
    case TransportRole:
        return dev.transport;
    case StatusRole:
        return dev.status;
    case BootModeRole:
        return dev.bootMode;
    case ConfidenceRole:
        return dev.confidence;
    case ConnectedRole:
        return dev.connected;
    case IconRole:
        return {};
    default:
        return {};
    }
}

QHash<int, QByteArray> DeviceListModel::roleNames() const
{
    return {
        {IdRole, "deviceId"},
        {NameRole, "name"},
        {VendorRole, "vendor"},
        {ProtocolRole, "protocol"},
        {TransportRole, "transport"},
        {StatusRole, "status"},
        {BootModeRole, "bootMode"},
        {ConfidenceRole, "confidence"},
        {ConnectedRole, "connected"},
        {IconRole, "icon"}
    };
}

void DeviceListModel::setDevices(const QList<DeviceInfo> &devices)
{
    beginResetModel();
    m_devices = devices;
    endResetModel();
    emit devicesChanged();
}

void DeviceListModel::addDevice(const DeviceInfo &device)
{
    auto sz = static_cast<int>(m_devices.size());
    beginInsertRows({}, sz, sz);
    m_devices.append(device);
    endInsertRows();
    emit devicesChanged();
}

void DeviceListModel::removeDevice(const QString &id)
{
    int sz = static_cast<int>(m_devices.size());
    for (int i = 0; i < sz; ++i) {
        if (m_devices[i].id == id) {
            beginRemoveRows({}, i, i);
            m_devices.removeAt(i);
            endRemoveRows();
            emit devicesChanged();
            return;
        }
    }
}

void DeviceListModel::updateDevice(const DeviceInfo &device)
{
    int sz = static_cast<int>(m_devices.size());
    for (int i = 0; i < sz; ++i) {
        if (m_devices[i].id == device.id) {
            m_devices[i] = device;
            emit dataChanged(index(i), index(i));
            emit devicesChanged();
            return;
        }
    }
    addDevice(device);
}

void DeviceListModel::clear()
{
    beginResetModel();
    m_devices.clear();
    endResetModel();
    emit devicesChanged();
}

DeviceInfo DeviceListModel::deviceAt(int row) const
{
    if (row >= 0 && row < static_cast<int>(m_devices.size()))
        return m_devices.at(row);
    return {};
}

DeviceInfo DeviceListModel::deviceById(const QString &id) const
{
    for (const auto &d : m_devices) {
        if (d.id == id)
            return d;
    }
    return {};
}
