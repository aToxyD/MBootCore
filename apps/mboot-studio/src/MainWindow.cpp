#include "gui/framework/MainWindow.hpp"
#include "gui/framework/ApplicationController.hpp"
#include "gui/framework/WindowStateManager.hpp"
#include "gui/framework/ThemeManager.hpp"
#include "gui/framework/SettingsManager.hpp"
#include "gui/framework/ActionManager.hpp"
#include "gui/framework/NotificationCenter.hpp"
#include "gui/framework/AboutDialog.hpp"
#include "gui/framework/UpdateChecker.hpp"
#include "gui/runtime/RuntimeBridge.hpp"
#include "gui/runtime/RuntimeEventDispatcher.hpp"
#include "gui/runtime/RuntimeModels.hpp"
#include "gui/runtime/RuntimeErrorMapper.hpp"
#include "gui/discovery/DeviceDiscoveryWidget.hpp"
#include "gui/session/SessionWidget.hpp"
#include "gui/firmware/PackageExplorer.hpp"
#include "gui/flash/FlashWidget.hpp"
#include "gui/gpt/PartitionTableWidget.hpp"
#include "gui/workflow/WorkflowDesigner.hpp"
#include "gui/job/JobQueueWidget.hpp"
#include "gui/plugin/PluginManagerWidget.hpp"
#include "gui/vendor/VendorManagerWidget.hpp"
#include "gui/transport/TransportMonitorWidget.hpp"
#include "gui/logs/LogViewerWidget.hpp"
#include "gui/settings/SettingsDialog.hpp"
#include "gui/diagnostics/DiagnosticsWidget.hpp"
#include "gui/devtools/DeveloperToolsWidget.hpp"
#include <QStackedWidget>
#include <QStatusBar>
#include <QMenuBar>
#include <QToolBar>
#include <QDockWidget>
#include <QCloseEvent>
#include <QMessageBox>
#include <QLabel>
#include <QApplication>

MainWindow::MainWindow(ApplicationController *ctrl, QWidget *parent)
    : QMainWindow(parent), m_controller(ctrl)
{
    setWindowTitle("MBoot Studio");
    setAccessibleName("MBoot Studio");
    setAccessibleDescription("Main application window for MBoot Studio firmware flashing tool");
    resize(1280, 800);
    setMinimumSize(1024, 600);

    QApplication::setDesktopFileName("mboot-studio");
}

MainWindow::~MainWindow() = default;

void MainWindow::setupUi()
{
    createCentralWidget();
    setupDockWidgets();
    setupMenus();
    setupToolBars();
    setupStatusBar();
    connectSignals();
    connectRuntimeBridge();
}

void MainWindow::createCentralWidget()
{
    m_centralStack = new QStackedWidget(this);

    m_discoveryWidget = new DeviceDiscoveryWidget(this);
    m_sessionWidget = new SessionWidget(this);
    m_packageExplorer = new PackageExplorer(this);
    m_flashWidget = new FlashWidget(this);
    m_partitionWidget = new PartitionTableWidget(this);
    m_workflowDesigner = new WorkflowDesigner(this);
    m_jobQueueWidget = new JobQueueWidget(this);
    m_pluginManagerWidget = new PluginManagerWidget(this);
    m_vendorManagerWidget = new VendorManagerWidget(this);
    m_transportWidget = new TransportMonitorWidget(this);
    m_logViewer = new LogViewerWidget(this);
    m_diagnosticsWidget = new DiagnosticsWidget(this);
    m_devToolsWidget = new DeveloperToolsWidget(this);

    m_centralStack->addWidget(m_discoveryWidget);
    m_centralStack->addWidget(m_sessionWidget);
    m_centralStack->addWidget(m_packageExplorer);
    m_centralStack->addWidget(m_flashWidget);
    m_centralStack->addWidget(m_partitionWidget);
    m_centralStack->addWidget(m_workflowDesigner);
    m_centralStack->addWidget(m_jobQueueWidget);
    m_centralStack->addWidget(m_pluginManagerWidget);
    m_centralStack->addWidget(m_vendorManagerWidget);
    m_centralStack->addWidget(m_transportWidget);
    m_centralStack->addWidget(m_logViewer);
    m_centralStack->addWidget(m_diagnosticsWidget);
    m_centralStack->addWidget(m_devToolsWidget);

    m_discoveryWidget->setAccessibleName("Device Discovery");
    m_discoveryWidget->setAccessibleDescription("Discover and connect to devices");
    m_sessionWidget->setAccessibleName("Session Manager");
    m_sessionWidget->setAccessibleDescription("Manage device sessions and connections");
    m_packageExplorer->setAccessibleName("Firmware Explorer");
    m_packageExplorer->setAccessibleDescription("Browse and inspect firmware packages");
    m_flashWidget->setAccessibleName("Flash Operations");
    m_flashWidget->setAccessibleDescription("Flash firmware to connected devices");
    m_partitionWidget->setAccessibleName("GPT Partition Manager");
    m_partitionWidget->setAccessibleDescription("View and manage GPT partition tables");
    m_workflowDesigner->setAccessibleName("Workflow Designer");
    m_workflowDesigner->setAccessibleDescription("Design and execute device workflows");
    m_jobQueueWidget->setAccessibleName("Job Queue");
    m_jobQueueWidget->setAccessibleDescription("View and manage queued jobs");
    m_pluginManagerWidget->setAccessibleName("Plugin Manager");
    m_pluginManagerWidget->setAccessibleDescription("Manage installed plugins and capabilities");
    m_vendorManagerWidget->setAccessibleName("Vendor Manager");
    m_vendorManagerWidget->setAccessibleDescription("Manage vendor integrations");
    m_transportWidget->setAccessibleName("Transport Monitor");
    m_transportWidget->setAccessibleDescription("Monitor USB, Serial, and TCP transport statistics");
    m_logViewer->setAccessibleName("Log Viewer");
    m_logViewer->setAccessibleDescription("View application logs with filtering and export");
    m_diagnosticsWidget->setAccessibleName("Diagnostics");
    m_diagnosticsWidget->setAccessibleDescription("Run diagnostics and view system health");
    m_devToolsWidget->setAccessibleName("Developer Tools");
    m_devToolsWidget->setAccessibleDescription("Inspector, console, profiler, and memory tools");

    setCentralWidget(m_centralStack);
}

void MainWindow::setupDockWidgets()
{
    // All views are in central stack; dock widgets can be added for specific panels
}

void MainWindow::createDockWidget(const QString &title, QWidget *widget,
                                   Qt::DockWidgetArea area, const QString &objectName)
{
    auto *dock = new QDockWidget(title, this);
    dock->setObjectName(objectName);
    dock->setWidget(widget);
    dock->setAllowedAreas(Qt::AllDockWidgetAreas);
    addDockWidget(area, dock);
    m_controller->windowStateManager()->registerDockWidget(objectName, dock);
}

void MainWindow::setupMenus()
{
    auto *actionMgr = m_controller->actionManager();
    auto *fileMenu = actionMgr->createMenu("&File", menuBar());
    auto *viewMenu = actionMgr->createMenu("&View", menuBar());
    auto *toolsMenu = actionMgr->createMenu("&Tools", menuBar());
    auto *helpMenu = actionMgr->createMenu("&Help", menuBar());

    // File menu
    auto *aboutAct = actionMgr->createAction("about", "&About MBoot Studio");
    aboutAct->setToolTip("Show information about MBoot Studio");
    connect(aboutAct, &QAction::triggered, this, [this]() {
        AboutDialog dlg(this);
        dlg.exec();
    });

    auto *quitAct = actionMgr->createAction("quit", "&Quit", QKeySequence::Quit);
    quitAct->setToolTip("Quit MBoot Studio");
    connect(quitAct, &QAction::triggered, this, &QWidget::close);

    fileMenu->addAction(aboutAct);
    fileMenu->addSeparator();
    fileMenu->addAction(quitAct);

    // View menu
    auto *discoveryAct = actionMgr->createAction("viewDiscovery", "&Discovery", QKeySequence(Qt::CTRL | Qt::Key_1));
    discoveryAct->setToolTip("Switch to Device Discovery view");
    connect(discoveryAct, &QAction::triggered, this, &MainWindow::switchToDiscovery);
    auto *sessionAct = actionMgr->createAction("viewSession", "&Session", QKeySequence(Qt::CTRL | Qt::Key_2));
    sessionAct->setToolTip("Switch to Session Management view");
    connect(sessionAct, &QAction::triggered, this, &MainWindow::switchToSession);
    auto *firmwareAct = actionMgr->createAction("viewFirmware", "&Firmware", QKeySequence(Qt::CTRL | Qt::Key_3));
    firmwareAct->setToolTip("Switch to Firmware Explorer view");
    connect(firmwareAct, &QAction::triggered, this, &MainWindow::switchToFirmware);
    auto *flashAct = actionMgr->createAction("viewFlash", "&Flash", QKeySequence(Qt::CTRL | Qt::Key_4));
    flashAct->setToolTip("Switch to Flash Operations view");
    connect(flashAct, &QAction::triggered, this, &MainWindow::switchToFlash);
    auto *gptAct = actionMgr->createAction("viewGPT", "&GPT", QKeySequence(Qt::CTRL | Qt::Key_5));
    gptAct->setToolTip("Switch to GPT Partition Manager view");
    connect(gptAct, &QAction::triggered, this, &MainWindow::switchToGPT);
    auto *workflowAct = actionMgr->createAction("viewWorkflow", "&Workflow", QKeySequence(Qt::CTRL | Qt::Key_6));
    workflowAct->setToolTip("Switch to Workflow Designer view");
    connect(workflowAct, &QAction::triggered, this, &MainWindow::switchToWorkflow);
    auto *jobsAct = actionMgr->createAction("viewJobs", "&Jobs", QKeySequence(Qt::CTRL | Qt::Key_7));
    jobsAct->setToolTip("Switch to Job Queue view");
    connect(jobsAct, &QAction::triggered, this, &MainWindow::switchToJobs);
    auto *pluginsAct = actionMgr->createAction("viewPlugins", "&Plugins", QKeySequence(Qt::CTRL | Qt::Key_8));
    pluginsAct->setToolTip("Switch to Plugin Manager view");
    connect(pluginsAct, &QAction::triggered, this, &MainWindow::switchToPlugins);
    auto *vendorsAct = actionMgr->createAction("viewVendors", "&Vendors", QKeySequence(Qt::CTRL | Qt::Key_9));
    vendorsAct->setToolTip("Switch to Vendor Manager view");
    connect(vendorsAct, &QAction::triggered, this, &MainWindow::switchToVendors);
    auto *transportAct = actionMgr->createAction("viewTransport", "&Transport", QKeySequence(Qt::CTRL | Qt::Key_0));
    transportAct->setToolTip("Switch to Transport Monitor view");
    connect(transportAct, &QAction::triggered, this, &MainWindow::switchToTransport);
    auto *logsAct = actionMgr->createAction("viewLogs", "&Logs");
    logsAct->setToolTip("Switch to Log Viewer view");
    connect(logsAct, &QAction::triggered, this, &MainWindow::switchToLogs);
    auto *settingsAct = actionMgr->createAction("viewSettings", "&Settings", QKeySequence::Preferences);
    settingsAct->setToolTip("Open Application Settings");
    connect(settingsAct, &QAction::triggered, this, &MainWindow::switchToSettings);
    auto *diagnosticsAct = actionMgr->createAction("viewDiagnostics", "&Diagnostics");
    diagnosticsAct->setToolTip("Switch to Diagnostics view");
    connect(diagnosticsAct, &QAction::triggered, this, &MainWindow::switchToDiagnostics);
    auto *devToolsAct = actionMgr->createAction("viewDevTools", "Developer &Tools");
    devToolsAct->setToolTip("Switch to Developer Tools view");
    connect(devToolsAct, &QAction::triggered, this, &MainWindow::switchToDevTools);

    viewMenu->addAction(discoveryAct);
    viewMenu->addAction(sessionAct);
    viewMenu->addAction(firmwareAct);
    viewMenu->addAction(flashAct);
    viewMenu->addAction(gptAct);
    viewMenu->addSeparator();
    viewMenu->addAction(workflowAct);
    viewMenu->addAction(jobsAct);
    viewMenu->addSeparator();
    viewMenu->addAction(pluginsAct);
    viewMenu->addAction(vendorsAct);
    viewMenu->addAction(transportAct);
    viewMenu->addAction(logsAct);
    viewMenu->addSeparator();
    viewMenu->addAction(settingsAct);
    viewMenu->addAction(diagnosticsAct);
    viewMenu->addAction(devToolsAct);

    // Tools menu
    auto *discoverAct = actionMgr->createAction("discover", "&Discover Devices", QKeySequence(Qt::Key_F5));
    discoverAct->setToolTip("Start scanning for connected devices");
    connect(discoverAct, &QAction::triggered, this, [this]() { m_discoveryWidget->startDiscovery(); });
    toolsMenu->addAction(discoverAct);

    // Help menu
    auto *checkUpdateAct = actionMgr->createAction("checkUpdate", "Check for &Updates");
    checkUpdateAct->setToolTip("Check for application updates");
    connect(checkUpdateAct, &QAction::triggered, this, [this]() {
        auto *checker = new UpdateChecker(this);
        checker->checkForUpdates();
    });
    helpMenu->addAction(checkUpdateAct);
}

void MainWindow::setupToolBars()
{
    auto *mainToolBar = addToolBar("Main");
    mainToolBar->setObjectName("mainToolBar");
    mainToolBar->setAccessibleName("Main Toolbar");
    mainToolBar->setToolTip("Navigate between application views");

    auto *actionMgr = m_controller->actionManager();
    mainToolBar->addAction(actionMgr->findAction("viewDiscovery"));
    mainToolBar->addAction(actionMgr->findAction("viewSession"));
    mainToolBar->addAction(actionMgr->findAction("viewFirmware"));
    mainToolBar->addAction(actionMgr->findAction("viewFlash"));
    mainToolBar->addAction(actionMgr->findAction("viewGPT"));
    mainToolBar->addSeparator();
    mainToolBar->addAction(actionMgr->findAction("viewWorkflow"));
    mainToolBar->addAction(actionMgr->findAction("viewJobs"));
}

void MainWindow::setupStatusBar()
{
    m_statusBar = statusBar();
    m_statusLabel = new QLabel("Ready");
    m_deviceStatusLabel = new QLabel("No device");
    m_connectionStatusLabel = new QLabel("Disconnected");

    m_statusLabel->setAccessibleName("Status");
    m_statusLabel->setAccessibleDescription("Current application status message");
    m_statusLabel->setToolTip("Displays the current application status");

    m_deviceStatusLabel->setAccessibleName("Device Count");
    m_deviceStatusLabel->setAccessibleDescription("Number of detected devices");
    m_deviceStatusLabel->setToolTip("Shows the number of detected devices");

    m_connectionStatusLabel->setAccessibleName("Connection Status");
    m_connectionStatusLabel->setAccessibleDescription("Current device connection status");
    m_connectionStatusLabel->setToolTip("Shows whether a device is connected");

    m_statusBar->addWidget(m_statusLabel, 1);
    m_statusBar->addPermanentWidget(m_deviceStatusLabel);
    m_statusBar->addPermanentWidget(m_connectionStatusLabel);
}

void MainWindow::connectSignals()
{
    auto *notif = m_controller->notificationCenter();
    connect(notif, &NotificationCenter::notificationPosted, this,
        [this](const NotificationCenter::Notification &n) {
            showNotification(n.title, n.message, n.severity);
        });
}

void MainWindow::connectRuntimeBridge()
{
    auto *bridge = m_controller->runtimeBridge();
    if (!bridge) return;

    m_discoveryWidget->setRuntimeBridge(bridge);
    m_sessionWidget->setRuntimeBridge(bridge);
    m_flashWidget->setRuntimeBridge(bridge);
    m_packageExplorer->setRuntimeBridge(bridge);
    m_diagnosticsWidget->setRuntimeBridge(bridge);
    m_pluginManagerWidget->setRuntimeBridge(bridge);

    auto *ed = bridge->eventDispatcher();
    connect(ed, &gui::runtime::RuntimeEventDispatcher::deviceListChanged,
            this, [this](const std::vector<gui::runtime::DeviceInfoView>& devices) {
                int count = static_cast<int>(devices.size());
                m_deviceStatusLabel->setText(
                    count > 0 ? QString("%1 device(s)").arg(count) : "No devices");
            });

    connect(ed, &gui::runtime::RuntimeEventDispatcher::sessionStatusChanged,
            this, [this](const gui::runtime::SessionInfoView& session) {
                if (session.isConnected) {
                    QString name = QString::fromStdString(session.deviceName);
                    m_connectionStatusLabel->setText("Connected: " + name);
                } else {
                    m_connectionStatusLabel->setText("Disconnected");
                }
            });

    connect(ed, &gui::runtime::RuntimeEventDispatcher::errorOccurred,
            this, [this](const gui::runtime::RuntimeError& error) {
                m_statusLabel->setText(
                    QString("[%1] %2")
                        .arg(error.severity == gui::runtime::ErrorSeverity::Critical ? "CRIT" :
                             error.severity == gui::runtime::ErrorSeverity::Error ? "ERR" :
                             error.severity == gui::runtime::ErrorSeverity::Warning ? "WARN" : "INFO")
                        .arg(QString::fromStdString(error.message)));
            });

    connect(ed, &gui::runtime::RuntimeEventDispatcher::flashOperationChanged,
            this, [this](const gui::runtime::FlashOperationView& op) {
                using S = gui::runtime::FlashStatus;
                switch (op.status) {
                    case S::Idle:
                        break;
                    case S::Preparing:
                    case S::Validating:
                        m_statusLabel->setText("Preparing flash...");
                        break;
                    case S::Flashing:
                        m_statusLabel->setText("Flashing in progress...");
                        break;
                    case S::Verifying:
                        m_statusLabel->setText("Verifying flash...");
                        break;
                    case S::Completed:
                        m_statusLabel->setText("Flash completed");
                        break;
                    case S::Cancelled:
                        m_statusLabel->setText("Flash cancelled");
                        break;
                    case S::Failed:
                        m_statusLabel->setText("Flash failed");
                        break;
                }
            });

    connect(ed, &gui::runtime::RuntimeEventDispatcher::flashProgressChanged,
            this, [this](const gui::runtime::FlashProgressView& progress) {
                m_statusLabel->setText(
                    QString("Flashing: %1% - %2")
                        .arg(progress.percentage, 0, 'f', 1)
                        .arg(QString::fromStdString(progress.currentOperation)));
            });
}

void MainWindow::showNotification(const QString &title, const QString &msg,
                                   NotificationCenter::Severity severity)
{
    m_statusLabel->setText(QString("[%1] %2: %3")
        .arg(severity == NotificationCenter::Error ? "ERR" :
             severity == NotificationCenter::Warning ? "WARN" :
             severity == NotificationCenter::Success ? "OK" : "INFO")
        .arg(title, msg));
}

void MainWindow::switchToDiscovery() { m_centralStack->setCurrentWidget(m_discoveryWidget); m_statusLabel->setText("Device Discovery"); }
void MainWindow::switchToSession() { m_centralStack->setCurrentWidget(m_sessionWidget); m_statusLabel->setText("Session Manager"); }
void MainWindow::switchToFirmware() { m_centralStack->setCurrentWidget(m_packageExplorer); m_statusLabel->setText("Firmware Explorer"); }
void MainWindow::switchToFlash() { m_centralStack->setCurrentWidget(m_flashWidget); m_statusLabel->setText("Flash Operations"); }
void MainWindow::switchToGPT() { m_centralStack->setCurrentWidget(m_partitionWidget); m_statusLabel->setText("GPT Manager"); }
void MainWindow::switchToWorkflow() { m_centralStack->setCurrentWidget(m_workflowDesigner); m_statusLabel->setText("Workflow Designer"); }
void MainWindow::switchToJobs() { m_centralStack->setCurrentWidget(m_jobQueueWidget); m_statusLabel->setText("Job Queue"); }
void MainWindow::switchToPlugins() { m_centralStack->setCurrentWidget(m_pluginManagerWidget); m_statusLabel->setText("Plugin Manager"); }
void MainWindow::switchToVendors() { m_centralStack->setCurrentWidget(m_vendorManagerWidget); m_statusLabel->setText("Vendor Manager"); }
void MainWindow::switchToTransport() { m_centralStack->setCurrentWidget(m_transportWidget); m_statusLabel->setText("Transport Monitor"); }
void MainWindow::switchToLogs() { m_centralStack->setCurrentWidget(m_logViewer); m_statusLabel->setText("Log Viewer"); }
void MainWindow::switchToSettings()
{
    if (!m_settingsDialog) m_settingsDialog = new SettingsDialog(this);
    m_settingsDialog->loadSettings();
    m_settingsDialog->exec();
}
void MainWindow::switchToDiagnostics() { m_centralStack->setCurrentWidget(m_diagnosticsWidget); m_statusLabel->setText("Diagnostics"); }
void MainWindow::switchToDevTools() { m_centralStack->setCurrentWidget(m_devToolsWidget); m_statusLabel->setText("Developer Tools"); }

void MainWindow::closeEvent(QCloseEvent *event)
{
    m_controller->shutdown();
    event->accept();
}

void MainWindow::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange) {
        // Retranslate UI
    }
    QMainWindow::changeEvent(event);
}
