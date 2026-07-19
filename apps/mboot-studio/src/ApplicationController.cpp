#include "gui/framework/ApplicationController.hpp"
#include "gui/runtime/RuntimeBridge.hpp"
#include "gui/framework/ThemeManager.hpp"
#include "gui/framework/SettingsManager.hpp"
#include "gui/framework/WindowStateManager.hpp"
#include "gui/framework/RecentFilesManager.hpp"
#include "gui/framework/ActionManager.hpp"
#include "gui/framework/ShortcutManager.hpp"
#include "gui/framework/CommandManager.hpp"
#include "gui/framework/NotificationCenter.hpp"
#include "gui/framework/MainWindow.hpp"
#include "gui/framework/SplashScreen.hpp"
#include "gui/framework/AboutDialog.hpp"
#include "gui/framework/UpdateChecker.hpp"
#include <QApplication>
#include <QTimer>
#include <QLocale>
#include <QFile>

ApplicationController::ApplicationController(QObject *parent)
    : QObject(parent) {}

ApplicationController::~ApplicationController() = default;

void ApplicationController::createManagers()
{
    m_themeManager = std::make_unique<ThemeManager>(this);
    m_settingsManager = std::make_unique<SettingsManager>(this);
    m_windowStateManager = std::make_unique<WindowStateManager>(this);
    m_recentFilesManager = std::make_unique<RecentFilesManager>(this);
    m_actionManager = std::make_unique<ActionManager>(this);
    m_shortcutManager = std::make_unique<ShortcutManager>(this);
    m_commandManager = std::make_unique<CommandManager>(this);
    m_notificationCenter = std::make_unique<NotificationCenter>(this);
    m_runtimeBridge = std::make_unique<gui::runtime::RuntimeBridge>(this);
}

bool ApplicationController::initializeManagers()
{
    auto theme = m_settingsManager->theme();
    m_themeManager->applyTheme(theme);

    auto accent = m_settingsManager->accentColor();
    if (accent.isValid())
        m_themeManager->setAccentColor(accent);

    m_shortcutManager->loadFromSettings();
    return true;
}

bool ApplicationController::initializeRuntime()
{
    auto result = m_runtimeBridge->initialize();
    return result.isOk();
}

void ApplicationController::createMainWindow()
{
    m_mainWindow = std::make_unique<MainWindow>(this);
    m_windowStateManager->registerMainWindow(m_mainWindow.get());
    m_mainWindow->setupUi();

    connect(m_themeManager.get(), &ThemeManager::themeApplied, this, [this](const QString &name) {
        m_settingsManager->setTheme(name);
        emit themeChanged(name);
    });
}

bool ApplicationController::initialize()
{
    createManagers();

    SplashScreen splash;
    splash.show();
    QApplication::processEvents();

    splash.showStatus("Initializing managers...");
    if (!initializeManagers()) return false;

    splash.showStatus("Initializing Runtime...");
    if (!initializeRuntime()) return false;

    splash.showStatus("Creating main window...");
    createMainWindow();
    splash.showStatus("Restoring state...");

    m_windowStateManager->restoreState();

    splash.finishWithDelay(m_mainWindow.get(), 300);
    emit initialized();
    return true;
}

int ApplicationController::run()
{
    m_mainWindow->show();
    return QApplication::exec();
}

void ApplicationController::shutdown()
{
    if (m_runtimeBridge) {
        m_runtimeBridge->shutdown();
    }
    m_windowStateManager->saveState();
    m_shortcutManager->saveToSettings();
    m_settingsManager->setWindowGeometry(m_mainWindow->saveGeometry());
    m_settingsManager->setWindowState(m_mainWindow->saveState());
    m_settingsManager->sync();
    emit shutdownRequested();
}
