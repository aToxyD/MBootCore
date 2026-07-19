#include "gui/flash/FlashPlanViewer.hpp"
#include <QVBoxLayout>
#include <QHeaderView>

class FlashPlanModel : public QAbstractItemModel {
public:
    explicit FlashPlanModel(const QVariantMap &plan, QObject *parent = nullptr)
        : QAbstractItemModel(parent), m_plan(plan) {}

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
        if (!parent.isValid()) return static_cast<int>(m_plan.size());
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
        auto keys = m_plan.keys();
        if (index.row() >= keys.size()) return {};
        if (index.column() == 0) return keys.at(index.row());
        return m_plan.value(keys.at(index.row())).toString();
    }

    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override
    {
        if (orientation != Qt::Horizontal || role != Qt::DisplayRole) return {};
        return section == 0 ? QStringLiteral("Key") : QStringLiteral("Value");
    }

private:
    QVariantMap m_plan;
};

FlashPlanViewer::FlashPlanViewer(QWidget *parent)
    : QWidget(parent)
{
    setupUi();
}

void FlashPlanViewer::setPlan(const QVariantMap &plan)
{
    m_treeView->setModel(new FlashPlanModel(plan, m_treeView));
}

void FlashPlanViewer::clear()
{
    m_treeView->setModel(nullptr);
}

void FlashPlanViewer::setupUi()
{
    auto *layout = new QVBoxLayout(this);
    m_treeView = new QTreeView(this);
    m_treeView->setAlternatingRowColors(true);
    layout->addWidget(m_treeView);
}
