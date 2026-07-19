#include "gui/vendor/VendorManagerWidget.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMessageBox>

VendorManagerWidget::VendorManagerWidget(QWidget *parent)
    : QWidget(parent)
    , m_table(new QTableWidget(this))
    , m_detailsBtn(new QPushButton(tr("Details"), this))
    , m_refreshBtn(new QPushButton(tr("Refresh"), this))
    , m_statusLabel(new QLabel(tr("Ready"), this))
{
    setupUi();
}

void VendorManagerWidget::setupUi()
{
    auto *layout = new QVBoxLayout(this);

    m_table->setAccessibleName("Vendor List");
    m_table->setAccessibleDescription("List of registered vendors with ID, name, and supported protocols");
    m_detailsBtn->setAccessibleName("Vendor Details");
    m_detailsBtn->setAccessibleDescription("View details of the selected vendor");
    m_detailsBtn->setToolTip("Show detailed information about the selected vendor");
    m_refreshBtn->setAccessibleName("Refresh Vendors");
    m_refreshBtn->setAccessibleDescription("Reload vendor list from the runtime");
    m_refreshBtn->setToolTip("Refresh the vendor list");
    m_refreshBtn->setDefault(true);
    m_statusLabel->setAccessibleName("Vendor Status");
    m_statusLabel->setAccessibleDescription("Current vendor management status");

    auto *btnLayout = new QHBoxLayout();
    btnLayout->addWidget(m_detailsBtn);
    btnLayout->addWidget(m_refreshBtn);
    layout->addLayout(btnLayout);

    layout->addWidget(m_table);
    layout->addWidget(m_statusLabel);

    m_table->setColumnCount(3);
    m_table->setHorizontalHeaderLabels({tr("ID"), tr("Name"), tr("Protocols")});
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);

    QWidget::setTabOrder(m_detailsBtn, m_refreshBtn);
    QWidget::setTabOrder(m_refreshBtn, m_table);

    connect(m_detailsBtn, &QPushButton::clicked, this, [this]() {
        auto selected = m_table->selectedItems();
        if (!selected.isEmpty())
            showVendorDetails(m_table->item(selected.first()->row(), 0)->text());
    });
    connect(m_refreshBtn, &QPushButton::clicked, this, &VendorManagerWidget::loadVendors);
}

void VendorManagerWidget::loadVendors()
{
    m_statusLabel->setText(tr("Vendors loaded"));
}

void VendorManagerWidget::showVendorDetails(const QString &vendorId)
{
    QMessageBox::information(this, tr("Vendor Details"),
        tr("Vendor ID: %1\nDetails not yet implemented.").arg(vendorId));
}
