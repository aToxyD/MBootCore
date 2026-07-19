#include "gui/discovery/DeviceTreeModel.hpp"

DeviceTreeModel::DeviceTreeModel(QObject *parent)
    : QAbstractItemModel(parent)
    , m_root(std::make_shared<TreeNode>())
{
    m_root->name = "Devices";
    m_root->type = "root";
}

DeviceTreeModel::~DeviceTreeModel() = default;

QModelIndex DeviceTreeModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!hasIndex(row, column, parent))
        return {};

    TreeNode *parentNode = parent.isValid()
        ? static_cast<TreeNode*>(parent.internalPointer())
        : m_root.get();

    if (row < parentNode->children.size())
        return createIndex(row, column, parentNode->children[row].get());

    return {};
}

QModelIndex DeviceTreeModel::parent(const QModelIndex &index) const
{
    if (!index.isValid())
        return {};

    auto *childNode = static_cast<TreeNode*>(index.internalPointer());
    auto *parentNode = childNode->parent;

    if (!parentNode || parentNode == m_root.get())
        return {};

    auto *grandparent = parentNode->parent;
    if (!grandparent)
        return {};

    for (int i = 0; i < grandparent->children.size(); ++i) {
        if (grandparent->children[i].get() == parentNode)
            return createIndex(i, 0, parentNode);
    }

    return {};
}

int DeviceTreeModel::rowCount(const QModelIndex &parent) const
{
    TreeNode *node = parent.isValid()
        ? static_cast<TreeNode*>(parent.internalPointer())
        : m_root.get();
    return static_cast<int>(node->children.size());
}

int DeviceTreeModel::columnCount(const QModelIndex &) const
{
    return 1;
}

QVariant DeviceTreeModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return {};

    auto *node = static_cast<TreeNode*>(index.internalPointer());

    switch (role) {
    case Qt::DisplayRole:
        return node->name;
    case Qt::UserRole:
        return QVariant::fromValue(node);
    default:
        return {};
    }
}

Qt::ItemFlags DeviceTreeModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

void DeviceTreeModel::setDevices(const QList<DeviceInfo> &devices)
{
    beginResetModel();
    m_root->children.clear();

    for (const auto &dev : devices) {
        auto vendor = findOrCreateVendor(dev.vendor.isEmpty() ? "Unknown" : dev.vendor);
        auto protocol = findOrCreateProtocol(vendor->name, dev.protocol.isEmpty() ? "Unknown" : dev.protocol);

        auto deviceNode = std::make_shared<TreeNode>();
        deviceNode->name = dev.name.isEmpty() ? dev.id : dev.name;
        deviceNode->type = "device";
        deviceNode->id = dev.id;
        deviceNode->parent = protocol.get();
        protocol->children.append(deviceNode);
    }

    endResetModel();
}

void DeviceTreeModel::clear()
{
    beginResetModel();
    m_root->children.clear();
    endResetModel();
}

std::shared_ptr<DeviceTreeModel::TreeNode>
DeviceTreeModel::findOrCreateVendor(const QString &name)
{
    for (const auto &child : m_root->children) {
        if (child->type == "vendor" && child->name == name)
            return child;
    }

    auto node = std::make_shared<TreeNode>();
    node->name = name;
    node->type = "vendor";
    node->parent = m_root.get();
    m_root->children.append(node);
    return node;
}

std::shared_ptr<DeviceTreeModel::TreeNode>
DeviceTreeModel::findOrCreateProtocol(const QString &vendor, const QString &protocol)
{
    auto vendorNode = findOrCreateVendor(vendor);

    for (const auto &child : vendorNode->children) {
        if (child->type == "protocol" && child->name == protocol)
            return child;
    }

    auto node = std::make_shared<TreeNode>();
    node->name = protocol;
    node->type = "protocol";
    node->parent = vendorNode.get();
    vendorNode->children.append(node);
    return node;
}
