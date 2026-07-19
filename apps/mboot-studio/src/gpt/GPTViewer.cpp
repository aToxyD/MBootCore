#include "gui/gpt/GPTViewer.hpp"
#include <QVBoxLayout>
#include <QHeaderView>

GPTViewer::GPTViewer(QWidget *parent)
    : QWidget(parent)
{
    setupUi();
}

void GPTViewer::setPartitions(const QVariantList &partitions)
{
    m_table->setRowCount(0);
    for (const auto &p : partitions) {
        auto map = p.toMap();
        int row = m_table->rowCount();
        m_table->insertRow(row);
        m_table->setItem(row, 0, new QTableWidgetItem(map.value("name").toString()));
        m_table->setItem(row, 1, new QTableWidgetItem(map.value("start").toString()));
        m_table->setItem(row, 2, new QTableWidgetItem(map.value("end").toString()));
        m_table->setItem(row, 3, new QTableWidgetItem(map.value("type").toString()));
    }
}

void GPTViewer::setupUi()
{
    auto *layout = new QVBoxLayout(this);

    m_diskCombo = new QComboBox(this);
    layout->addWidget(m_diskCombo);

    m_table = new QTableWidget(0, 4, this);
    m_table->setHorizontalHeaderLabels({tr("Name"), tr("Start"), tr("End"), tr("Type")});
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    layout->addWidget(m_table);

    connect(m_table, &QTableWidget::cellClicked, this, [this](int row, int) {
        emit partitionSelected(row);
    });
}
