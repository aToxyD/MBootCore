#include "gui/job/SchedulerView.hpp"
#include <QVBoxLayout>
#include <QHeaderView>
#include <QLabel>

SchedulerView::SchedulerView(QWidget *parent)
    : QWidget(parent)
    , m_table(new QTableWidget(this))
{
    setupUi();
}

void SchedulerView::setupUi()
{
    auto *layout = new QVBoxLayout(this);
    layout->addWidget(new QLabel(tr("Schedule"), this));
    layout->addWidget(m_table);

    m_table->setColumnCount(3);
    m_table->setHorizontalHeaderLabels({tr("Time"), tr("Job"), tr("Priority")});
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
}

void SchedulerView::setSchedule(const QVariantList &schedule)
{
    int sz = static_cast<int>(schedule.size());
    m_table->setRowCount(sz);
    for (int i = 0; i < sz; ++i) {
        QVariantMap entry = schedule[i].toMap();
        m_table->setItem(i, 0, new QTableWidgetItem(entry.value("time").toString()));
        m_table->setItem(i, 1, new QTableWidgetItem(entry.value("job").toString()));
        m_table->setItem(i, 2, new QTableWidgetItem(entry.value("priority").toString()));
    }
}

void SchedulerView::clear()
{
    m_table->setRowCount(0);
}
