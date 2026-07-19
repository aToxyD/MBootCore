#include "gui/discovery/DeviceDetailsWidget.hpp"
#include "gui/discovery/DeviceListModel.hpp"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QPushButton>
#include <QFrame>

DeviceDetailsWidget::DeviceDetailsWidget(QWidget *parent)
    : QWidget(parent)
    , m_nameLabel(new QLabel)
    , m_vendorLabel(new QLabel)
    , m_protocolLabel(new QLabel)
    , m_transportLabel(new QLabel)
    , m_statusLabel(new QLabel)
    , m_bootModeLabel(new QLabel)
    , m_confidenceLabel(new QLabel)
    , m_iconLabel(new QLabel)
    , m_connectBtn(new QPushButton("Connect"))
    , m_disconnectBtn(new QPushButton("Disconnect"))
{
    setupUi();

    connect(m_connectBtn, &QPushButton::clicked, this, [this]() {
        if (!m_currentDeviceId.isEmpty())
            emit connectRequested(m_currentDeviceId);
    });

    connect(m_disconnectBtn, &QPushButton::clicked, this, [this]() {
        if (!m_currentDeviceId.isEmpty())
            emit disconnectRequested(m_currentDeviceId);
    });
}

void DeviceDetailsWidget::setupUi()
{
    auto *mainLayout = new QVBoxLayout(this);

    auto *header = new QLabel("Device Details");
    header->setStyleSheet("font-weight: bold; font-size: 14px;");
    mainLayout->addWidget(header);

    auto *sep = new QFrame;
    sep->setFrameShape(QFrame::HLine);
    mainLayout->addWidget(sep);

    auto *form = new QFormLayout;
    form->addRow("Name:", m_nameLabel);
    form->addRow("Vendor:", m_vendorLabel);
    form->addRow("Protocol:", m_protocolLabel);
    form->addRow("Transport:", m_transportLabel);
    form->addRow("Status:", m_statusLabel);
    form->addRow("Boot Mode:", m_bootModeLabel);
    form->addRow("Confidence:", m_confidenceLabel);
    mainLayout->addLayout(form);

    mainLayout->addStretch();

    auto *btnLayout = new QHBoxLayout;
    m_connectBtn->setEnabled(false);
    m_disconnectBtn->setEnabled(false);
    btnLayout->addWidget(m_connectBtn);
    btnLayout->addWidget(m_disconnectBtn);
    mainLayout->addLayout(btnLayout);

    clear();
}

void DeviceDetailsWidget::showDevice(const DeviceInfo &device)
{
    m_currentDeviceId = device.id;

    m_nameLabel->setText(device.name);
    m_vendorLabel->setText(device.vendor);
    m_protocolLabel->setText(device.protocol);
    m_transportLabel->setText(device.transport);
    m_statusLabel->setText(device.status);
    m_bootModeLabel->setText(device.bootMode);
    m_confidenceLabel->setText(QString::number(device.confidence));

    m_connectBtn->setEnabled(true);
    m_disconnectBtn->setEnabled(device.connected);
}

void DeviceDetailsWidget::clear()
{
    m_currentDeviceId.clear();
    m_nameLabel->setText("-");
    m_vendorLabel->setText("-");
    m_protocolLabel->setText("-");
    m_transportLabel->setText("-");
    m_statusLabel->setText("-");
    m_bootModeLabel->setText("-");
    m_confidenceLabel->setText("-");
    m_connectBtn->setEnabled(false);
    m_disconnectBtn->setEnabled(false);
}
