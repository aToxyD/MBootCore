#include "gui/discovery/DeviceFilterProxyModel.hpp"
#include "gui/discovery/DeviceListModel.hpp"

DeviceFilterProxyModel::DeviceFilterProxyModel(QObject *parent)
    : QSortFilterProxyModel(parent)
{
    setFilterCaseSensitivity(Qt::CaseInsensitive);
    setSortRole(DeviceListModel::ConfidenceRole);
    sort(0, Qt::DescendingOrder);
}

void DeviceFilterProxyModel::setFilterText(const QString &text)
{
    m_filterText = text;
    invalidateFilter();
}

void DeviceFilterProxyModel::setVendorFilter(const QString &vendor)
{
    m_vendorFilter = vendor;
    invalidateFilter();
}

void DeviceFilterProxyModel::setProtocolFilter(const QString &protocol)
{
    m_protocolFilter = protocol;
    invalidateFilter();
}

void DeviceFilterProxyModel::setStatusFilter(const QString &status)
{
    m_statusFilter = status;
    invalidateFilter();
}

void DeviceFilterProxyModel::setShowConnectedOnly(bool only)
{
    m_connectedOnly = only;
    invalidateFilter();
}

bool DeviceFilterProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    auto *model = sourceModel();
    if (!model)
        return true;

    auto nameIdx = model->index(sourceRow, 0, sourceParent);
    auto name = model->data(nameIdx, DeviceListModel::NameRole).toString();
    auto vendor = model->data(nameIdx, DeviceListModel::VendorRole).toString();
    auto protocol = model->data(nameIdx, DeviceListModel::ProtocolRole).toString();
    auto status = model->data(nameIdx, DeviceListModel::StatusRole).toString();
    bool connected = model->data(nameIdx, DeviceListModel::ConnectedRole).toBool();

    if (m_connectedOnly && !connected)
        return false;

    if (!m_vendorFilter.isEmpty() && vendor != m_vendorFilter)
        return false;

    if (!m_protocolFilter.isEmpty() && protocol != m_protocolFilter)
        return false;

    if (!m_statusFilter.isEmpty() && status != m_statusFilter)
        return false;

    if (!m_filterText.isEmpty()) {
        bool match = name.contains(m_filterText, Qt::CaseInsensitive)
                  || vendor.contains(m_filterText, Qt::CaseInsensitive)
                  || protocol.contains(m_filterText, Qt::CaseInsensitive)
                  || status.contains(m_filterText, Qt::CaseInsensitive);
        if (!match)
            return false;
    }

    return true;
}

bool DeviceFilterProxyModel::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
    auto *model = sourceModel();
    if (!model)
        return QSortFilterProxyModel::lessThan(left, right);

    int leftConf = model->data(left, DeviceListModel::ConfidenceRole).toInt();
    int rightConf = model->data(right, DeviceListModel::ConfidenceRole).toInt();

    if (leftConf != rightConf)
        return leftConf < rightConf;

    return QSortFilterProxyModel::lessThan(left, right);
}
