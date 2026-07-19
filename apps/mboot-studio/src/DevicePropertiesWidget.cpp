#include "gui/discovery/DevicePropertiesWidget.hpp"
#include "gui/discovery/DeviceListModel.hpp"

#include <QVBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QFrame>

DevicePropertiesWidget::DevicePropertiesWidget(QWidget *parent)
    : QWidget(parent)
    , m_table(new QTableWidget)
{
    setupUi();
}

void DevicePropertiesWidget::setupUi()
{
    auto *layout = new QVBoxLayout(this);

    auto *header = new QLabel("Device Properties");
    header->setStyleSheet("font-weight: bold; font-size: 14px;");
    layout->addWidget(header);

    auto *sep = new QFrame;
    sep->setFrameShape(QFrame::HLine);
    layout->addWidget(sep);

    m_table->setColumnCount(2);
    m_table->setHorizontalHeaderLabels({"Property", "Value"});
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->verticalHeader()->setVisible(false);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setAlternatingRowColors(true);
    layout->addWidget(m_table);
}

void DevicePropertiesWidget::showProperties(const DeviceInfo &device)
{
    m_table->setRowCount(0);

    struct Entry { QString property; QString value; };
    const Entry entries[] = {
        {"ID", device.id},
        {"Name", device.name},
        {"Vendor", device.vendor},
        {"Protocol", device.protocol},
        {"Transport", device.transport},
        {"Status", device.status},
        {"Boot Mode", device.bootMode},
        {"Confidence", QString::number(device.confidence)},
        {"Connected", device.connected ? "Yes" : "No"}
    };

    m_table->setRowCount(9);
    for (int i = 0; i < 9; ++i) {
        auto *propItem = new QTableWidgetItem(entries[i].property);
        auto *valItem = new QTableWidgetItem(entries[i].value);
        m_table->setItem(i, 0, propItem);
        m_table->setItem(i, 1, valItem);
    }

    m_table->resizeColumnsToContents();
}

void DevicePropertiesWidget::clear()
{
    m_table->setRowCount(0);
}
