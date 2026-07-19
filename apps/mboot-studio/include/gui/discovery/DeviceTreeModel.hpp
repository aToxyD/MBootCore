#pragma once

#include <QAbstractItemModel>
#include <QList>
#include <QString>
#include <QIcon>
#include <memory>

#include "DeviceListModel.hpp"

class DeviceTreeModel : public QAbstractItemModel {
    Q_OBJECT
public:
    struct TreeNode {
        QString name;
        QString type; // "vendor", "protocol", "device"
        QString id;
        QList<std::shared_ptr<TreeNode>> children;
        TreeNode *parent{nullptr};
    };

    explicit DeviceTreeModel(QObject *parent = nullptr);
    ~DeviceTreeModel() override;

    QModelIndex index(int row, int column, const QModelIndex &parent = {}) const override;
    QModelIndex parent(const QModelIndex &index) const override;
    int rowCount(const QModelIndex &parent = {}) const override;
    int columnCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;

    void setDevices(const QList<DeviceInfo> &devices);
    void clear();

private:
    std::shared_ptr<TreeNode> findOrCreateVendor(const QString &name);
    std::shared_ptr<TreeNode> findOrCreateProtocol(const QString &vendor, const QString &protocol);

    std::shared_ptr<TreeNode> m_root;
};
