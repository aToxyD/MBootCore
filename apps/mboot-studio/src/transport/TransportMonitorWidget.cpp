#include "gui/transport/TransportMonitorWidget.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QPushButton>
#include <QGroupBox>

TransportMonitorWidget::TransportMonitorWidget(QWidget *parent)
    : QWidget(parent)
    , m_tabs(new QTabWidget(this))
    , m_latencyLabel(new QLabel("0 ms", this))
    , m_bandwidthLabel(new QLabel("0 B/s", this))
    , m_packetsLabel(new QLabel("0", this))
    , m_errorsLabel(new QLabel("0", this))
    , m_reconnectLabel(new QLabel("0", this))
    , m_resetBtn(new QPushButton(tr("Reset Stats"), this))
{
    setupUi();
}

void TransportMonitorWidget::setupUi()
{
    m_tabs->setAccessibleName("Transport Statistics");
    m_tabs->setAccessibleDescription("View transport statistics for USB, Serial, and TCP connections");
    m_resetBtn->setAccessibleName("Reset Statistics");
    m_resetBtn->setAccessibleDescription("Reset all transport statistics to zero");
    m_resetBtn->setToolTip("Clear all transport statistics counters");
    m_resetBtn->setDefault(true);

    auto *layout = new QVBoxLayout(this);

    auto createStatsWidget = [&](const QString &title) -> QWidget * {
        auto *w = new QWidget(this);
        w->setAccessibleName(title + " Statistics");
        w->setAccessibleDescription("Transport statistics for " + title + " connections");
        auto *f = new QFormLayout(w);
        m_latencyLabel->setAccessibleName(title + " Latency");
        m_bandwidthLabel->setAccessibleName(title + " Bandwidth");
        m_packetsLabel->setAccessibleName(title + " Packets");
        m_errorsLabel->setAccessibleName(title + " Errors");
        m_reconnectLabel->setAccessibleName(title + " Reconnects");
        f->addRow(tr("Latency"), m_latencyLabel);
        f->addRow(tr("Bandwidth"), m_bandwidthLabel);
        f->addRow(tr("Packets"), m_packetsLabel);
        f->addRow(tr("Errors"), m_errorsLabel);
        f->addRow(tr("Reconnects"), m_reconnectLabel);
        return w;
    };

    m_tabs->addTab(createStatsWidget("USB"), tr("USB"));
    m_tabs->addTab(createStatsWidget("Serial"), tr("Serial"));
    m_tabs->addTab(createStatsWidget("TCP"), tr("TCP"));
    layout->addWidget(m_tabs);

    auto *btnLayout = new QHBoxLayout();
    btnLayout->addStretch();
    btnLayout->addWidget(m_resetBtn);
    layout->addLayout(btnLayout);

    connect(m_resetBtn, &QPushButton::clicked, this, &TransportMonitorWidget::resetStats);
}

void TransportMonitorWidget::setUsbStats(const QVariantMap &stats)
{
    Q_UNUSED(stats)
}

void TransportMonitorWidget::setSerialStats(const QVariantMap &stats)
{
    Q_UNUSED(stats)
}

void TransportMonitorWidget::setTcpStats(const QVariantMap &stats)
{
    Q_UNUSED(stats)
}

void TransportMonitorWidget::resetStats()
{
    m_latencyLabel->setText("0 ms");
    m_bandwidthLabel->setText("0 B/s");
    m_packetsLabel->setText("0");
    m_errorsLabel->setText("0");
    m_reconnectLabel->setText("0");
}
