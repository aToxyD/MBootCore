#include "gui/flash/PartitionSelection.hpp"
#include <QVBoxLayout>
#include <QHeaderView>

PartitionSelection::PartitionSelection(QWidget *parent)
    : QWidget(parent)
{
    setupUi();
}

void PartitionSelection::setPartitions(const QVariantList &partitions)
{
    m_table->setRowCount(0);
    m_table->blockSignals(true);
    for (const auto &p : partitions) {
        auto map = p.toMap();
        int row = m_table->rowCount();
        m_table->insertRow(row);
        m_table->setItem(row, 1, new QTableWidgetItem(map.value("name").toString()));
        m_table->setItem(row, 2, new QTableWidgetItem(map.value("size").toString()));
        auto *check = new QTableWidgetItem();
        check->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        check->setCheckState(Qt::Checked);
        m_table->setItem(row, 0, check);
    }
    m_table->blockSignals(false);
}

QStringList PartitionSelection::selectedPartitions() const
{
    QStringList sel;
    for (int i = 0; i < m_table->rowCount(); ++i) {
        auto *item = m_table->item(i, 0);
        if (item && item->checkState() == Qt::Checked)
            sel.append(m_table->item(i, 1)->text());
    }
    return sel;
}

void PartitionSelection::selectAll(bool selected)
{
    for (int i = 0; i < m_table->rowCount(); ++i) {
        auto *item = m_table->item(i, 0);
        if (item)
            item->setCheckState(selected ? Qt::Checked : Qt::Unchecked);
    }
}

void PartitionSelection::setupUi()
{
    auto *layout = new QVBoxLayout(this);

    m_selectAllCheck = new QCheckBox(tr("Select All"), this);
    m_selectAllCheck->setChecked(true);
    layout->addWidget(m_selectAllCheck);

    m_table = new QTableWidget(0, 3, this);
    m_table->setHorizontalHeaderLabels({tr(""), tr("Partition"), tr("Size")});
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    layout->addWidget(m_table);

    connect(m_selectAllCheck, &QCheckBox::toggled, this, &PartitionSelection::selectAll);
    connect(m_table, &QTableWidget::itemChanged, this, [this]() {
        emit selectionChanged(selectedPartitions());
    });
}
