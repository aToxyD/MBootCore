#pragma once

#include <QMainWindow>
#include <QLabel>
#include <QStackedWidget>
#include <memory>
#include "gui/framework/NotificationCenter.hpp"

class ApplicationController;
class NotificationCenter;

class DeviceDiscoveryWidget;
class SessionWidget;
class PackageExplorer;
class FlashWidget;
class PartitionTableWidget;
class WorkflowDesigner;
class JobQueueWidget;
class PluginManagerWidget;
class VendorManagerWidget;
class TransportMonitorWidget;
class LogViewerWidget;
class SettingsDialog;
class DiagnosticsWidget;
class DeveloperToolsWidget;

class QStackedWidget;
class QStatusBar;
class QMenuBar;
class QToolBar;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(ApplicationController *ctrl, QWidget *parent = nullptr);
    ~MainWindow() override;

    void setupUi();
    void setupDockWidgets();
    void setupMenus();
    void setupToolBars();
    void setupStatusBar();
    void restoreState();

    ApplicationController *controller() const { return m_controller; }

public slots:
    void switchToDiscovery();
    void switchToSession();
    void switchToFirmware();
    void switchToFlash();
    void switchToGPT();
    void switchToWorkflow();
    void switchToJobs();
    void switchToPlugins();
    void switchToVendors();
    void switchToTransport();
    void switchToLogs();
    void switchToSettings();
    void switchToDiagnostics();
    void switchToDevTools();

protected:
    void closeEvent(QCloseEvent *event) override;
    void changeEvent(QEvent *event) override;

private:
    void createCentralWidget();
    void createDockWidget(const QString &title, QWidget *widget, Qt::DockWidgetArea area, const QString &objectName);
    void connectSignals();
    void connectRuntimeBridge();
    void showNotification(const QString &title, const QString &msg, NotificationCenter::Severity severity);

    ApplicationController *m_controller;
    QStackedWidget *m_centralStack;

    // Views
    DeviceDiscoveryWidget *m_discoveryWidget;
    SessionWidget *m_sessionWidget;
    PackageExplorer *m_packageExplorer;
    FlashWidget *m_flashWidget;
    PartitionTableWidget *m_partitionWidget;
    WorkflowDesigner *m_workflowDesigner;
    JobQueueWidget *m_jobQueueWidget;
    PluginManagerWidget *m_pluginManagerWidget;
    VendorManagerWidget *m_vendorManagerWidget;
    TransportMonitorWidget *m_transportWidget;
    LogViewerWidget *m_logViewer;
    SettingsDialog *m_settingsDialog;
    DiagnosticsWidget *m_diagnosticsWidget;
    DeveloperToolsWidget *m_devToolsWidget;

    QStatusBar *m_statusBar;
    QLabel *m_statusLabel;
    QLabel *m_deviceStatusLabel;
    QLabel *m_connectionStatusLabel;
};
