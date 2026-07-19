#include "gui/firmware/PartitionViewer.hpp"
#include <QVBoxLayout>
#include <QHeaderView>

PartitionViewer::PartitionViewer(QWidget *parent)
    : QWidget(parent)
{
    setupUi();
}

void PartitionViewer::setPartitions(const QVariantList &partitions)
{
    m_table->setRowCount(0);
    for (const auto &p : partitions) {
        auto map = p.toMap();
        int row = m_table->rowCount();
        m_table->insertRow(row);
        m_table->setItem(row, 0, new QTableWidgetItem(map.value("name").toString()));
        m_table->setItem(row, 1, new QTableWidgetItem(map.value("size").toString()));
        m_table->setItem(row, 2, new QTableWidgetItem(map.value("type").toString()));
    }
}

void PartitionViewer::clear()
{
    m_table->setRowCount(0);
}

void PartitionViewer::setupUi()
{
    auto *layout = new QVBoxLayout(this);
    m_table = new QTableWidget(0, 3, this);
    m_table->setHorizontalHeaderLabels({tr("Name"), tr("Size"), tr("Type")});
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    layout->addWidget(m_table);
}
