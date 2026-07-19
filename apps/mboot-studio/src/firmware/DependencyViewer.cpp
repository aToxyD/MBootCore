#include "gui/firmware/DependencyViewer.hpp"
#include <QVBoxLayout>

class DependencyModel : public QAbstractItemModel {
public:
    explicit DependencyModel(const QVariantMap &deps, QObject *parent = nullptr)
        : QAbstractItemModel(parent), m_deps(deps) {}

    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override
    {
        Q_UNUSED(column)
        if (!hasIndex(row, column, parent)) return {};
        return createIndex(row, 0, reinterpret_cast<void*>(static_cast<quintptr>(parent.isValid() ? parent.row() + 1 : 0)));
    }

    QModelIndex parent(const QModelIndex &child) const override
    {
        Q_UNUSED(child)
        return {};
    }

    int rowCount(const QModelIndex &parent = QModelIndex()) const override
    {
        if (!parent.isValid()) return static_cast<int>(m_deps.size());
        return 0;
    }

    int columnCount(const QModelIndex &parent = QModelIndex()) const override
    {
        Q_UNUSED(parent)
        return 2;
    }

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override
    {
        if (!index.isValid() || role != Qt::DisplayRole) return {};
        auto keys = m_deps.keys();
        if (index.row() >= keys.size()) return {};
        if (index.column() == 0) return keys.at(index.row());
        return m_deps.value(keys.at(index.row())).toString();
    }

    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override
    {
        if (orientation != Qt::Horizontal || role != Qt::DisplayRole) return {};
        return section == 0 ? QStringLiteral("Key") : QStringLiteral("Value");
    }

private:
    QVariantMap m_deps;
};

void DependencyViewer::setDependencies(const QVariantMap &deps)
{
    m_treeView->setModel(new DependencyModel(deps, m_treeView));
}

void DependencyViewer::clear()
{
    m_treeView->setModel(nullptr);
}

DependencyViewer::DependencyViewer(QWidget *parent)
    : QWidget(parent)
{
    setupUi();
}

void DependencyViewer::setupUi()
{
    auto *layout = new QVBoxLayout(this);
    m_treeView = new QTreeView(this);
    m_treeView->setAlternatingRowColors(true);
    layout->addWidget(m_treeView);
}
