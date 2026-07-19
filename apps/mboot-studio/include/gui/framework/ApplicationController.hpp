#pragma once

#include <QObject>
#include <memory>

class ThemeManager;
class SettingsManager;
class WindowStateManager;
class RecentFilesManager;
class ActionManager;
class ShortcutManager;
class CommandManager;
class NotificationCenter;
class MainWindow;

namespace gui::runtime { class RuntimeBridge; }

class ApplicationController : public QObject {
    Q_OBJECT
public:
    explicit ApplicationController(QObject *parent = nullptr);
    ~ApplicationController() override;

    bool initialize();
    int run();
    void shutdown();

    ThemeManager *themeManager() const { return m_themeManager.get(); }
    SettingsManager *settingsManager() const { return m_settingsManager.get(); }
    WindowStateManager *windowStateManager() const { return m_windowStateManager.get(); }
    RecentFilesManager *recentFilesManager() const { return m_recentFilesManager.get(); }
    ActionManager *actionManager() const { return m_actionManager.get(); }
    ShortcutManager *shortcutManager() const { return m_shortcutManager.get(); }
    CommandManager *commandManager() const { return m_commandManager.get(); }
    NotificationCenter *notificationCenter() const { return m_notificationCenter.get(); }
    gui::runtime::RuntimeBridge *runtimeBridge() const { return m_runtimeBridge.get(); }

signals:
    void initialized();
    void shutdownRequested();
    void themeChanged(const QString &theme);

private:
    void createManagers();
    bool initializeManagers();
    bool initializeRuntime();
    void createMainWindow();

    std::unique_ptr<ThemeManager> m_themeManager;
    std::unique_ptr<SettingsManager> m_settingsManager;
    std::unique_ptr<WindowStateManager> m_windowStateManager;
    std::unique_ptr<RecentFilesManager> m_recentFilesManager;
    std::unique_ptr<ActionManager> m_actionManager;
    std::unique_ptr<ShortcutManager> m_shortcutManager;
    std::unique_ptr<CommandManager> m_commandManager;
    std::unique_ptr<NotificationCenter> m_notificationCenter;
    std::unique_ptr<MainWindow> m_mainWindow;
    std::unique_ptr<gui::runtime::RuntimeBridge> m_runtimeBridge;
};
