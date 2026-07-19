#include "gui/job/RollbackViewer.hpp"
#include <QVBoxLayout>
#include <QHeaderView>
#include <QLabel>

RollbackViewer::RollbackViewer(QWidget *parent)
    : QWidget(parent)
    , m_table(new QTableWidget(this))
{
    setupUi();
}

void RollbackViewer::setupUi()
{
    auto *layout = new QVBoxLayout(this);
    layout->addWidget(new QLabel(tr("Rollback Plan"), this));
    layout->addWidget(m_table);

    m_table->setColumnCount(3);
    m_table->setHorizontalHeaderLabels({tr("Step"), tr("Action"), tr("Status")});
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
}

void RollbackViewer::setRollbackPlan(const QVariantList &plan)
{
    int sz = static_cast<int>(plan.size());
    m_table->setRowCount(sz);
    for (int i = 0; i < sz; ++i) {
        QVariantMap entry = plan[i].toMap();
        m_table->setItem(i, 0, new QTableWidgetItem(entry.value("step").toString()));
        m_table->setItem(i, 1, new QTableWidgetItem(entry.value("action").toString()));
        m_table->setItem(i, 2, new QTableWidgetItem(entry.value("status").toString()));
    }
}

void RollbackViewer::clear()
{
    m_table->setRowCount(0);
}
