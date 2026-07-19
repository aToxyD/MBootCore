#pragma once

#include <QSortFilterProxyModel>
#include <QString>

class DeviceFilterProxyModel : public QSortFilterProxyModel {
    Q_OBJECT
public:
    explicit DeviceFilterProxyModel(QObject *parent = nullptr);

    void setFilterText(const QString &text);
    void setVendorFilter(const QString &vendor);
    void setProtocolFilter(const QString &protocol);
    void setStatusFilter(const QString &status);
    void setShowConnectedOnly(bool only);

    QString filterText() const { return m_filterText; }

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;
    bool lessThan(const QModelIndex &left, const QModelIndex &right) const override;

private:
    QString m_filterText;
    QString m_vendorFilter;
    QString m_protocolFilter;
    QString m_statusFilter;
    bool m_connectedOnly{false};
};
