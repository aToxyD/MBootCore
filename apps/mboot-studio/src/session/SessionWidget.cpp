#include "gui/session/SessionWidget.hpp"
#include "gui/session/SessionMonitor.hpp"
#include "gui/session/SessionStatistics.hpp"
#include "gui/runtime/RuntimeBridge.hpp"
#include "gui/runtime/RuntimeEventDispatcher.hpp"
#include "gui/runtime/RuntimeModels.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>

SessionWidget::SessionWidget(QWidget *parent)
    : QWidget(parent)
{
    setupUi();
}

void SessionWidget::setRuntimeBridge(gui::runtime::RuntimeBridge *bridge)
{
    if (m_runtimeBridge == bridge) return;

    if (m_runtimeBridge) {
        auto *ed = m_runtimeBridge->eventDispatcher();
        disconnect(ed, &gui::runtime::RuntimeEventDispatcher::sessionStatusChanged,
                   this, &SessionWidget::onSessionStatusChanged);
        disconnect(m_runtimeBridge, &gui::runtime::RuntimeBridge::connectionSucceeded,
                   this, &SessionWidget::onConnectionSucceeded);
        disconnect(m_runtimeBridge, &gui::runtime::RuntimeBridge::disconnected,
                   this, &SessionWidget::onDisconnected);
    }

    m_runtimeBridge = bridge;

    if (m_runtimeBridge) {
        connectRuntimeBridge();
    }
}

void SessionWidget::connectRuntimeBridge()
{
    if (!m_runtimeBridge) return;

    auto *ed = m_runtimeBridge->eventDispatcher();
    connect(ed, &gui::runtime::RuntimeEventDispatcher::sessionStatusChanged,
            this, &SessionWidget::onSessionStatusChanged);
    connect(m_runtimeBridge, &gui::runtime::RuntimeBridge::connectionSucceeded,
            this, &SessionWidget::onConnectionSucceeded);
    connect(m_runtimeBridge, &gui::runtime::RuntimeBridge::disconnected,
            this, &SessionWidget::onDisconnected);
}

void SessionWidget::setupUi()
{
    auto *mainLayout = new QVBoxLayout(this);

    m_tabs = new QTabWidget(this);
    m_tabs->setAccessibleName("Session Tabs");
    m_tabs->setAccessibleDescription("Monitor session activity and view statistics");
    m_monitor = new SessionMonitor(this);
    m_stats = new SessionStatistics(this);
    m_tabs->addTab(m_monitor, tr("Monitor"));
    m_tabs->addTab(m_stats, tr("Statistics"));
    mainLayout->addWidget(m_tabs);

    auto *btnLayout = new QHBoxLayout();
    m_connectBtn = new QPushButton(tr("Connect"), this);
    m_disconnectBtn = new QPushButton(tr("Disconnect"), this);
    m_pauseBtn = new QPushButton(tr("Pause"), this);
    m_resumeBtn = new QPushButton(tr("Resume"), this);
    m_cancelBtn = new QPushButton(tr("Cancel"), this);

    m_connectBtn->setAccessibleName("Connect");
    m_connectBtn->setAccessibleDescription("Establish connection to the selected device");
    m_connectBtn->setToolTip("Connect to the selected device");
    m_connectBtn->setDefault(true);

    m_disconnectBtn->setAccessibleName("Disconnect");
    m_disconnectBtn->setAccessibleDescription("Disconnect from the current device");
    m_disconnectBtn->setToolTip("Disconnect from the active device");

    m_pauseBtn->setAccessibleName("Pause");
    m_pauseBtn->setAccessibleDescription("Pause current device operation");
    m_pauseBtn->setToolTip("Pause the current read/write operation");

    m_resumeBtn->setAccessibleName("Resume");
    m_resumeBtn->setAccessibleDescription("Resume paused device operation");
    m_resumeBtn->setToolTip("Resume a paused operation");

    m_cancelBtn->setAccessibleName("Cancel");
    m_cancelBtn->setAccessibleDescription("Cancel the current device operation");
    m_cancelBtn->setToolTip("Cancel the current operation");

    btnLayout->addWidget(m_connectBtn);
    btnLayout->addWidget(m_disconnectBtn);
    btnLayout->addWidget(m_pauseBtn);
    btnLayout->addWidget(m_resumeBtn);
    btnLayout->addWidget(m_cancelBtn);
    mainLayout->addLayout(btnLayout);

    m_statusLabel = new QLabel(tr("Disconnected"), this);
    m_statusLabel->setAccessibleName("Session Status");
    m_statusLabel->setAccessibleDescription("Current session connection status");
    mainLayout->addWidget(m_statusLabel);

    m_connectBtn->setEnabled(false);
    m_disconnectBtn->setEnabled(false);
    m_pauseBtn->setEnabled(false);
    m_resumeBtn->setEnabled(false);
    m_cancelBtn->setEnabled(false);

    connect(m_connectBtn, &QPushButton::clicked, this, &SessionWidget::connectRequested);
    connect(m_disconnectBtn, &QPushButton::clicked, this, &SessionWidget::disconnectRequested);
    connect(m_pauseBtn, &QPushButton::clicked, this, &SessionWidget::pauseRequested);
    connect(m_resumeBtn, &QPushButton::clicked, this, &SessionWidget::resumeRequested);
    connect(m_cancelBtn, &QPushButton::clicked, this, &SessionWidget::cancelRequested);

    QWidget::setTabOrder(m_connectBtn, m_disconnectBtn);
    QWidget::setTabOrder(m_disconnectBtn, m_pauseBtn);
    QWidget::setTabOrder(m_pauseBtn, m_resumeBtn);
    QWidget::setTabOrder(m_resumeBtn, m_cancelBtn);
}

void SessionWidget::onSessionStatusChanged(const gui::runtime::SessionInfoView& session)
{
    bool connected = session.isConnected;
    bool busy = (session.status == gui::runtime::SessionStatus::Busy ||
                 session.status == gui::runtime::SessionStatus::Reading ||
                 session.status == gui::runtime::SessionStatus::Writing ||
                 session.status == gui::runtime::SessionStatus::Erasing);

    m_connectBtn->setEnabled(!connected && !busy);
    m_disconnectBtn->setEnabled(connected && !busy);
    m_pauseBtn->setEnabled(busy);
    m_resumeBtn->setEnabled(false);
    m_cancelBtn->setEnabled(busy);

    QString statusText;
    if (connected) {
        switch (session.status) {
            case gui::runtime::SessionStatus::Idle:
                statusText = tr("Connected: %1").arg(QString::fromStdString(session.deviceName));
                break;
            case gui::runtime::SessionStatus::Busy:
                statusText = tr("Connected: %1 (Busy)").arg(QString::fromStdString(session.deviceName));
                break;
            case gui::runtime::SessionStatus::Reading:
                statusText = tr("Connected: %1 (Reading)").arg(QString::fromStdString(session.deviceName));
                break;
            case gui::runtime::SessionStatus::Writing:
                statusText = tr("Connected: %1 (Writing)").arg(QString::fromStdString(session.deviceName));
                break;
            case gui::runtime::SessionStatus::Erasing:
                statusText = tr("Connected: %1 (Erasing)").arg(QString::fromStdString(session.deviceName));
                break;
            case gui::runtime::SessionStatus::Error:
                statusText = tr("Connected: %1 (Error)").arg(QString::fromStdString(session.deviceName));
                break;
        }
    } else {
        statusText = tr("Disconnected");
    }

    m_statusLabel->setText(statusText);
    m_monitor->appendLog(statusText, connected ? "info" : "warning");
}

void SessionWidget::onConnectionSucceeded(const QString& connectionPath)
{
    m_monitor->appendLog(tr("Device connected: %1").arg(connectionPath), "info");
}

void SessionWidget::onDisconnected()
{
    m_monitor->appendLog(tr("Device disconnected"), "info");
}
