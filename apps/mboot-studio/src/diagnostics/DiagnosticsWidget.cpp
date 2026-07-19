#include "gui/diagnostics/DiagnosticsWidget.hpp"
#include "gui/runtime/RuntimeBridge.hpp"
#include "gui/runtime/RuntimeEventDispatcher.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QTextEdit>
#include <QLabel>
#include <QTableWidget>
#include <QHeaderView>
#include <QDateTime>
#include <QFileDialog>
#include <QFile>

DiagnosticsWidget::DiagnosticsWidget(QWidget *parent)
    : QWidget(parent)
{
    setupUi();
}

void DiagnosticsWidget::setRuntimeBridge(gui::runtime::RuntimeBridge *bridge)
{
    if (m_bridge) return;
    m_bridge = bridge;

    auto *dispatcher = m_bridge->eventDispatcher();
    connect(dispatcher, &gui::runtime::RuntimeEventDispatcher::diagnosticsUpdated,
            this, &DiagnosticsWidget::onDiagnosticsUpdated);
    connect(dispatcher, &gui::runtime::RuntimeEventDispatcher::healthChanged,
            this, &DiagnosticsWidget::onHealthChanged);
}

void DiagnosticsWidget::runAll()
{
    if (m_bridge) {
        m_bridge->refreshDiagnostics();
    } else {
        appendOutput(tr("[%1] No runtime connected")
            .arg(QDateTime::currentDateTime().toString(Qt::ISODate)));
    }
}

QString DiagnosticsWidget::buildReport(const gui::runtime::RuntimeDiagnosticsView &diagnostics)
{
    QString report;
    report += "=== MBoot Studio Diagnostics Report ===\n";
    report += QString("Timestamp: %1\n").arg(QDateTime::currentDateTime().toString(Qt::ISODate));
    report += QString("Version: %1\n").arg(QString::fromStdString(diagnostics.version));
    report += "\n--- Runtime Health ---\n";
    report += QString("Active Sessions: %1\n").arg(diagnostics.health.activeSessions);
    report += QString("Active Workflows: %1\n").arg(diagnostics.health.activeWorkflows);
    report += QString("Loaded Plugins: %1\n").arg(diagnostics.health.loadedPlugins);
    report += QString("Connected Devices: %1\n").arg(diagnostics.health.connectedDevices);
    report += QString("Memory Usage: %1 bytes\n").arg(diagnostics.health.memoryUsageBytes);
    report += QString("Thread Count: %1\n").arg(diagnostics.health.threadCount);
    report += "\n--- Statistics ---\n";
    report += QString("Total Errors: %1\n").arg(diagnostics.totalErrors);
    report += QString("Total Recoveries: %1\n").arg(diagnostics.totalRecoveries);
    report += QString("Jobs Executed: %1\n").arg(diagnostics.jobsExecuted);
    report += "\n--- Devices ---\n";
    for (const auto& dev : diagnostics.devices) {
        report += QString("  %1 [%2] - %3\n")
            .arg(QString::fromStdString(dev.name))
            .arg(QString::fromStdString(dev.transportType))
            .arg(dev.connected ? "Connected" : "Disconnected");
    }
    if (!diagnostics.failures.empty()) {
        report += "\n--- Failures ---\n";
        for (const auto& f : diagnostics.failures)
            report += QString("  - %1\n").arg(QString::fromStdString(f));
    }
    return report;
}

bool DiagnosticsWidget::saveReport(const QString &path, const QString &report)
{
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        return false;
    file.write(report.toUtf8());
    return true;
}

void DiagnosticsWidget::exportReport()
{
    if (m_lastDiagnostics.health.activeSessions == 0 &&
        m_lastDiagnostics.devices.empty() &&
        m_lastDiagnostics.failures.empty()) {
        runAll();
    }
    QString path = QFileDialog::getSaveFileName(this, tr("Export Report"),
        QString(), tr("Text Files (*.txt);;All Files (*)"));
    if (path.isEmpty())
        return;
    if (saveReport(path, buildReport(m_lastDiagnostics)))
        appendOutput(tr("Report exported to %1").arg(path));
}

void DiagnosticsWidget::onDiagnosticsUpdated(const gui::runtime::RuntimeDiagnosticsView &diagnostics)
{
    m_lastDiagnostics = diagnostics;
    updateRuntimeTab(diagnostics);
    updateTransportTab(diagnostics);
    updatePluginTab(diagnostics);
    updateEnvironmentTab(diagnostics);
    appendOutput(tr("[%1] Diagnostics refreshed")
        .arg(QDateTime::currentDateTime().toString(Qt::ISODate)));
}

void DiagnosticsWidget::onHealthChanged(const gui::runtime::RuntimeHealthView &health)
{
    m_lastDiagnostics.health = health;
}

void DiagnosticsWidget::updateRuntimeTab(const gui::runtime::RuntimeDiagnosticsView &diagnostics)
{
    m_runtimeTable->setRowCount(0);
    auto addRow = [this](const QString& key, const QString& value) {
        int row = m_runtimeTable->rowCount();
        m_runtimeTable->insertRow(row);
        m_runtimeTable->setItem(row, 0, new QTableWidgetItem(key));
        m_runtimeTable->setItem(row, 1, new QTableWidgetItem(value));
    };
    addRow(tr("Version"), QString::fromStdString(diagnostics.version));
    addRow(tr("Uptime"), QString::number(diagnostics.health.uptimeSeconds, 'f', 1) + "s");
    addRow(tr("Active Sessions"), QString::number(diagnostics.health.activeSessions));
    addRow(tr("Active Workflows"), QString::number(diagnostics.health.activeWorkflows));
    addRow(tr("Queued Jobs"), QString::number(diagnostics.health.queuedJobs));
    addRow(tr("Connected Devices"), QString::number(diagnostics.health.connectedDevices));
    addRow(tr("Memory Usage"), QString::number(diagnostics.health.memoryUsageBytes) + " bytes");
    addRow(tr("Thread Count"), QString::number(diagnostics.health.threadCount));
    addRow(tr("Transport State"), QString::fromStdString(diagnostics.health.transportState));
    addRow(tr("Total Errors"), QString::number(diagnostics.totalErrors));
    addRow(tr("Total Recoveries"), QString::number(diagnostics.totalRecoveries));
    addRow(tr("Total Timeouts"), QString::number(diagnostics.totalTimeouts));
    addRow(tr("Total Disconnects"), QString::number(diagnostics.totalDisconnects));
    addRow(tr("Jobs Executed"), QString::number(diagnostics.jobsExecuted));
    addRow(tr("Workflows Executed"), QString::number(diagnostics.workflowsExecuted));
}

void DiagnosticsWidget::updateTransportTab(const gui::runtime::RuntimeDiagnosticsView &diagnostics)
{
    m_transportTable->setRowCount(0);
    auto addRow = [this](const QString& key, const QString& value) {
        int row = m_transportTable->rowCount();
        m_transportTable->insertRow(row);
        m_transportTable->setItem(row, 0, new QTableWidgetItem(key));
        m_transportTable->setItem(row, 1, new QTableWidgetItem(value));
    };
    addRow(tr("Avg Flash Speed"), QString::number(diagnostics.averageFlashSpeedBps, 'f', 2) + " B/s");
    addRow(tr("Avg Read Speed"), QString::number(diagnostics.averageReadSpeedBps, 'f', 2) + " B/s");
    addRow(tr("Avg Write Speed"), QString::number(diagnostics.averageWriteSpeedBps, 'f', 2) + " B/s");
    addRow(tr("Connected Devices"), QString::number(diagnostics.health.connectedDevices));
    for (size_t i = 0; i < diagnostics.devices.size(); ++i) {
        const auto& dev = diagnostics.devices[i];
        addRow(tr("Device %1").arg(i + 1),
            QString::fromStdString(dev.name + " [" + dev.transportType + "]"));
    }
}

void DiagnosticsWidget::updatePluginTab(const gui::runtime::RuntimeDiagnosticsView &diagnostics)
{
    m_pluginTable->setRowCount(0);
    auto addRow = [this](const QString& key, const QString& value) {
        int row = m_pluginTable->rowCount();
        m_pluginTable->insertRow(row);
        m_pluginTable->setItem(row, 0, new QTableWidgetItem(key));
        m_pluginTable->setItem(row, 1, new QTableWidgetItem(value));
    };
    addRow(tr("Loaded Plugins"), QString::number(diagnostics.health.loadedPlugins));
    addRow(tr("Loaded Vendors"), QString::number(diagnostics.health.loadedVendors));
}

void DiagnosticsWidget::updateEnvironmentTab(const gui::runtime::RuntimeDiagnosticsView &diagnostics)
{
    m_environmentTable->setRowCount(0);
    auto addRow = [this](const QString& key, const QString& value) {
        int row = m_environmentTable->rowCount();
        m_environmentTable->insertRow(row);
        m_environmentTable->setItem(row, 0, new QTableWidgetItem(key));
        m_environmentTable->setItem(row, 1, new QTableWidgetItem(value));
    };
    addRow(tr("OS Version"), QString::fromStdString(diagnostics.osInfo));
    addRow(tr("MBoot Version"), QString::fromStdString(diagnostics.version));
    addRow(tr("Failures"), QString::number(diagnostics.failures.size()));
    addRow(tr("Warnings"), QString::number(diagnostics.warnings.size()));
    addRow(tr("Recommendations"), QString::number(diagnostics.recommendations.size()));
    for (size_t i = 0; i < diagnostics.recommendations.size(); ++i) {
        addRow(tr("Recommendation %1").arg(i + 1),
            QString::fromStdString(diagnostics.recommendations[i]));
    }
}

void DiagnosticsWidget::appendOutput(const QString &text)
{
    m_outputView->append(text);
}

void DiagnosticsWidget::setupUi()
{
    auto *layout = new QVBoxLayout(this);

    auto *btnLayout = new QHBoxLayout();
    m_runBtn = new QPushButton(tr("Run All"), this);
    m_runBtn->setAccessibleName("Run All Diagnostics");
    m_runBtn->setAccessibleDescription("Execute all diagnostic checks across all categories");
    m_runBtn->setToolTip("Run all diagnostic checks");
    m_runBtn->setDefault(true);
    btnLayout->addWidget(m_runBtn);

    m_refreshBtn = new QPushButton(tr("Refresh"), this);
    m_refreshBtn->setAccessibleName("Refresh Diagnostics");
    m_refreshBtn->setAccessibleDescription("Refresh diagnostic data from the runtime");
    m_refreshBtn->setToolTip("Refresh diagnostic information");
    btnLayout->addWidget(m_refreshBtn);

    btnLayout->addStretch();

    m_exportBtn = new QPushButton(tr("Export"), this);
    m_exportBtn->setAccessibleName("Export Report");
    m_exportBtn->setAccessibleDescription("Export diagnostic report to a text file");
    m_exportBtn->setToolTip("Save diagnostic report to a file");
    btnLayout->addWidget(m_exportBtn);
    layout->addLayout(btnLayout);

    m_tabs = new QTabWidget(this);
    m_tabs->setAccessibleName("Diagnostics Categories");
    m_tabs->setAccessibleDescription("Navigate between Runtime, Transport, Plugin, and Environment diagnostics");

    m_runtimeTable = new QTableWidget(this);
    m_runtimeTable->setColumnCount(2);
    m_runtimeTable->setHorizontalHeaderLabels({tr("Property"), tr("Value")});
    m_runtimeTable->horizontalHeader()->setStretchLastSection(true);
    m_runtimeTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_runtimeTable->setAccessibleName("Runtime Health");
    m_runtimeTable->setAccessibleDescription("Runtime health metrics and statistics");
    m_tabs->addTab(m_runtimeTable, tr("Runtime"));

    m_transportTable = new QTableWidget(this);
    m_transportTable->setColumnCount(2);
    m_transportTable->setHorizontalHeaderLabels({tr("Property"), tr("Value")});
    m_transportTable->horizontalHeader()->setStretchLastSection(true);
    m_transportTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_transportTable->setAccessibleName("Transport Statistics");
    m_transportTable->setAccessibleDescription("Transport layer performance statistics");
    m_tabs->addTab(m_transportTable, tr("Transport"));

    m_pluginTable = new QTableWidget(this);
    m_pluginTable->setColumnCount(2);
    m_pluginTable->setHorizontalHeaderLabels({tr("Property"), tr("Value")});
    m_pluginTable->horizontalHeader()->setStretchLastSection(true);
    m_pluginTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_pluginTable->setAccessibleName("Plugin Status");
    m_pluginTable->setAccessibleDescription("Loaded plugins and vendors status");
    m_tabs->addTab(m_pluginTable, tr("Plugins"));

    m_environmentTable = new QTableWidget(this);
    m_environmentTable->setColumnCount(2);
    m_environmentTable->setHorizontalHeaderLabels({tr("Property"), tr("Value")});
    m_environmentTable->horizontalHeader()->setStretchLastSection(true);
    m_environmentTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_environmentTable->setAccessibleName("Environment Information");
    m_environmentTable->setAccessibleDescription("Operating system and application environment details");
    m_tabs->addTab(m_environmentTable, tr("Environment"));

    layout->addWidget(m_tabs);

    m_outputView = new QTextEdit(this);
    m_outputView->setReadOnly(true);
    m_outputView->setFont(QFont("Consolas", 9));
    m_outputView->setAccessibleName("Diagnostic Output");
    m_outputView->setAccessibleDescription("Diagnostic execution log and messages");
    m_outputView->setPlaceholderText(tr("Diagnostic output will appear here after running checks..."));
    auto *outputLabel = new QLabel(tr("Output"), this);
    outputLabel->setAccessibleName("Output Label");
    layout->addWidget(outputLabel);
    layout->addWidget(m_outputView);

    QWidget::setTabOrder(m_runBtn, m_refreshBtn);
    QWidget::setTabOrder(m_refreshBtn, m_exportBtn);
    QWidget::setTabOrder(m_exportBtn, m_tabs);

    connect(m_runBtn, &QPushButton::clicked, this, &DiagnosticsWidget::runAll);
    connect(m_refreshBtn, &QPushButton::clicked, this, &DiagnosticsWidget::runAll);
    connect(m_exportBtn, &QPushButton::clicked, this, &DiagnosticsWidget::exportReport);
}
