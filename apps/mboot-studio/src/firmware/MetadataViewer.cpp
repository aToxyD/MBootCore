#include "gui/firmware/MetadataViewer.hpp"
#include <QVBoxLayout>
#include <QHeaderView>

MetadataViewer::MetadataViewer(QWidget *parent)
    : QWidget(parent)
{
    setupUi();
}

void MetadataViewer::setMetadata(const QVariantMap &metadata)
{
    m_table->setRowCount(0);
    for (auto it = metadata.constBegin(); it != metadata.constEnd(); ++it) {
        int row = m_table->rowCount();
        m_table->insertRow(row);
        m_table->setItem(row, 0, new QTableWidgetItem(it.key()));
        m_table->setItem(row, 1, new QTableWidgetItem(it.value().toString()));
    }
}

void MetadataViewer::clear()
{
    m_table->setRowCount(0);
}

void MetadataViewer::setupUi()
{
    auto *layout = new QVBoxLayout(this);
    m_table = new QTableWidget(0, 2, this);
    m_table->setHorizontalHeaderLabels({tr("Key"), tr("Value")});
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    layout->addWidget(m_table);
}
