#include <QTest>
#include <QApplication>
#include <QSignalSpy>
#include <QMainWindow>

namespace { struct VersionSetter { VersionSetter() { QApplication::setApplicationVersion(MBOOTCORE_VERSION); } } _vs; }

#include "gui/framework/ThemeManager.hpp"
#include "gui/framework/SettingsManager.hpp"
#include "gui/framework/WindowStateManager.hpp"
#include "gui/framework/RecentFilesManager.hpp"
#include "gui/framework/ActionManager.hpp"
#include "gui/framework/ShortcutManager.hpp"
#include "gui/framework/CommandManager.hpp"
#include "gui/framework/NotificationCenter.hpp"
#include "gui/framework/SplashScreen.hpp"
#include "gui/framework/AboutDialog.hpp"
#include "gui/framework/UpdateChecker.hpp"

class FrameworkTest : public QObject {
    Q_OBJECT

private slots:
    void testThemeManager();
    void testSettingsManager();
    void testWindowStateManager();
    void testRecentFilesManager();
    void testActionManager();
    void testShortcutManager();
    void testCommandManager();
    void testNotificationCenter();
    void testSplashScreen();
    void testAboutDialog();
    void testUpdateChecker();
};

void FrameworkTest::testThemeManager()
{
    ThemeManager tm;
    QCOMPARE(tm.currentTheme(), ThemeManager::Dark);
    QVERIFY(!tm.availableThemes().isEmpty());

    tm.applyTheme(ThemeManager::Light);
    QCOMPARE(tm.currentTheme(), ThemeManager::Light);

    tm.applyTheme("High Contrast");
    QCOMPARE(tm.currentThemeName(), QString("High Contrast"));

    QSignalSpy spy(&tm, &ThemeManager::themeApplied);
    tm.applyTheme(ThemeManager::Classic);
    QCOMPARE(spy.count(), 1);

    QSignalSpy accentSpy(&tm, &ThemeManager::accentColorChanged);
    tm.setAccentColor(QColor("#ff0000"));
    QCOMPARE(accentSpy.count(), 1);
    QCOMPARE(tm.accentColor(), QColor("#ff0000"));

    QSignalSpy fontSpy(&tm, &ThemeManager::fontScaleChanged);
    tm.setFontScale(1.2);
    QCOMPARE(fontSpy.count(), 1);
    QVERIFY(qFuzzyCompare(tm.fontScale(), 1.2));

    // Verify palettes are created with expected colors
    QCOMPARE(tm.darkPalette().color(QPalette::Window), QColor(30, 30, 30));
    QCOMPARE(tm.lightPalette().color(QPalette::Window), QColor(240, 240, 240));
    QCOMPARE(tm.highContrastPalette().color(QPalette::Window), Qt::black);
    QVERIFY(tm.classicPalette().color(QPalette::Active, QPalette::Window).isValid());
}

void FrameworkTest::testSettingsManager()
{
    SettingsManager sm;
    sm.setValue("test/key", 42);
    QVERIFY(sm.contains("test/key"));
    QCOMPARE(sm.getValue("test/key").toInt(), 42);

    sm.removeValue("test/key");
    QVERIFY(!sm.contains("test/key"));

    sm.setTheme("Light");
    QCOMPARE(sm.theme(), QString("Light"));

    sm.setAccentColor(QColor("#00ff00"));
    QCOMPARE(sm.accentColor(), QColor("#00ff00"));

    sm.setDeveloperMode(true);
    QVERIFY(sm.developerMode());

    QSignalSpy spy(&sm, &SettingsManager::settingChanged);
    sm.setValue("test/signal", "value");
    QCOMPARE(spy.count(), 1);

    sm.sync(); // Should not crash
}

void FrameworkTest::testWindowStateManager()
{
    SettingsManager sm;
    WindowStateManager wsm(&sm);

    auto *window = new QMainWindow();
    wsm.registerMainWindow(window);

    QSignalSpy saveSpy(&wsm, &WindowStateManager::stateSaved);
    wsm.saveState();
    QCOMPARE(saveSpy.count(), 1);

    QSignalSpy restoreSpy(&wsm, &WindowStateManager::stateRestored);
    wsm.restoreState();
    QCOMPARE(restoreSpy.count(), 1);

    QSignalSpy layoutSpy(&wsm, &WindowStateManager::layoutChanged);
    wsm.saveLayout("TestPreset");
    wsm.restoreLayout("TestPreset");
    QCOMPARE(layoutSpy.count(), 1);

    QVERIFY(!wsm.availablePresets().isEmpty());

    wsm.resetLayout();
    QVERIFY(wsm.autoSave());
    wsm.setAutoSave(false);
    QVERIFY(!wsm.autoSave());

    delete window;
}

void FrameworkTest::testRecentFilesManager()
{
    SettingsManager sm;
    RecentFilesManager rfm(&sm);

    QSignalSpy spy(&rfm, &RecentFilesManager::recentFilesChanged);
    rfm.addFile("/path/to/file1.bin");
    QCOMPARE(spy.count(), 1);
    QCOMPARE(rfm.recentFiles().size(), 1);

    rfm.addFile("/path/to/file2.bin");
    QCOMPARE(rfm.recentFiles().size(), 2);
    QCOMPARE(rfm.recentFiles().first(), QString("/path/to/file2.bin"));

    QSignalSpy rmSpy(&rfm, &RecentFilesManager::fileRemoved);
    rfm.removeFile("/path/to/file1.bin");
    QCOMPARE(rmSpy.count(), 1);
    QCOMPARE(rfm.recentFiles().size(), 1);

    rfm.setMaxFiles(3);
    QCOMPARE(rfm.maxFiles(), 3);

    QSignalSpy clearSpy(&rfm, &RecentFilesManager::cleared);
    rfm.clear();
    QCOMPARE(clearSpy.count(), 1);
    QVERIFY(rfm.recentFiles().isEmpty());

    QVERIFY(rfm.recentFilesWithTimestamps().isEmpty());
}

void FrameworkTest::testActionManager()
{
    ActionManager am;

    auto *action = am.createAction("testAction", "&Test Action", QKeySequence("Ctrl+T"));
    QVERIFY(action != nullptr);
    QCOMPARE(action->text(), QString("&Test Action"));

    QCOMPARE(am.findAction("testAction"), action);
    QVERIFY(am.findAction("nonexistent") == nullptr);

    QSignalSpy spy(&am, &ActionManager::actionTriggered);
    action->trigger();
    QCOMPARE(spy.count(), 1);

    am.setActionText("testAction", "&Renamed");
    QCOMPARE(action->text(), QString("&Renamed"));

    am.setActionEnabled("testAction", false);
    QVERIFY(!action->isEnabled());

    am.setActionVisible("testAction", false);
    QVERIFY(!action->isVisible());

    QVERIFY(!am.actionIds().isEmpty());

    QVERIFY(am.createMenu("&File") != nullptr);
    QVERIFY(am.findMenu("&File") != nullptr);
    QVERIFY(am.createToolBar("Main") != nullptr);
}

void FrameworkTest::testShortcutManager()
{
    auto *window = new QMainWindow();
    ShortcutManager sm;

    QSignalSpy spy(&sm, &ShortcutManager::shortcutChanged);
    sm.registerShortcut("test", QKeySequence("Ctrl+S"), window, SLOT(close()));
    QCOMPARE(sm.shortcut("test"), QKeySequence("Ctrl+S"));

    sm.applyShortcut("test", QKeySequence("Ctrl+Shift+S"));
    QCOMPARE(spy.count(), 1);
    QCOMPARE(sm.shortcut("test"), QKeySequence("Ctrl+Shift+S"));

    sm.restoreDefaults();
    QCOMPARE(sm.shortcut("test"), QKeySequence("Ctrl+S"));

    QVERIFY(!sm.allShortcuts().isEmpty());

    sm.loadFromSettings();
    sm.saveToSettings();

    delete window;
}

void FrameworkTest::testCommandManager()
{
    CommandManager cm;

    QVERIFY(!cm.canUndo());
    QVERIFY(!cm.canRedo());

    QSignalSpy execSpy(&cm, &CommandManager::commandExecuted);
    cm.execute("cmd1");
    QCOMPARE(execSpy.count(), 1);
    QVERIFY(cm.canUndo());
    QVERIFY(!cm.canRedo());
    QCOMPARE(cm.lastCommand(), QString("cmd1"));

    cm.execute("cmd2");
    QCOMPARE(cm.commandHistory().size(), 2);

    QSignalSpy undoSpy(&cm, &CommandManager::commandUndone);
    cm.undo();
    QCOMPARE(undoSpy.count(), 1);
    QVERIFY(cm.canRedo());

    QSignalSpy redoSpy(&cm, &CommandManager::commandRedone);
    cm.redo();
    QCOMPARE(redoSpy.count(), 1);

    cm.setMaxHistory(10);
    QCOMPARE(cm.maxHistory(), 10);

    QSignalSpy clearSpy(&cm, &CommandManager::historyCleared);
    cm.clearHistory();
    QCOMPARE(clearSpy.count(), 1);
    QVERIFY(!cm.canUndo());
    QVERIFY(!cm.canRedo());
}

void FrameworkTest::testNotificationCenter()
{
    NotificationCenter nc;

    QSignalSpy postSpy(&nc, &NotificationCenter::notificationPosted);
    QString id = nc.post("Test Title", "Test Message", NotificationCenter::Warning);
    QCOMPARE(postSpy.count(), 1);
    QCOMPARE(nc.count(), 1);
    QCOMPARE(nc.unreadCount(), 1);

    QSignalSpy dismissSpy(&nc, &NotificationCenter::notificationDismissed);
    nc.dismiss(id);
    QCOMPARE(dismissSpy.count(), 1);

    nc.post("Info", "Info msg", NotificationCenter::Info);
    nc.post("Error", "Error msg", NotificationCenter::Error);
    nc.post("Success", "Success msg", NotificationCenter::Success);
    QCOMPARE(nc.count(), 4);

    QCOMPARE(nc.activeNotifications().size(), 3); // 1 dismissed

    QSignalSpy allDismissSpy(&nc, &NotificationCenter::allDismissed);
    nc.dismissAll();
    QCOMPARE(allDismissSpy.count(), 1);

    QVERIFY(nc.activeNotifications().isEmpty());

    nc.post("New", "New msg");
    nc.markAllRead();
    QCOMPARE(nc.unreadCount(), 0);

    QSignalSpy clearSpy(&nc, &NotificationCenter::cleared);
    QSignalSpy unreadSpy(&nc, &NotificationCenter::unreadCountChanged);
    nc.clear();
    QCOMPARE(clearSpy.count(), 1);
    QCOMPARE(nc.count(), 0);
}

void FrameworkTest::testSplashScreen()
{
    SplashScreen splash;
    QSignalSpy statusSpy(&splash, &SplashScreen::statusChanged);
    splash.showStatus("Loading...");
    QCOMPARE(statusSpy.count(), 1);

    QSignalSpy progressSpy(&splash, &SplashScreen::progressChanged);
    splash.setProgress(50);
    QCOMPARE(progressSpy.count(), 1);
}

void FrameworkTest::testAboutDialog()
{
    AboutDialog dlg;
    QCOMPARE(dlg.windowTitle(), QString("About MBoot Studio"));
    QVERIFY(dlg.width() > 0);
}

void FrameworkTest::testUpdateChecker()
{
    UpdateChecker checker;
    QVERIFY(!checker.currentVersion().isEmpty());
    QVERIFY(!checker.isUpdateAvailable());

    QSignalSpy startSpy(&checker, &UpdateChecker::checkStarted);
    QSignalSpy finishSpy(&checker, &UpdateChecker::checkFinished);
    checker.checkForUpdates();
    QCOMPARE(startSpy.count(), 1);
}

QTEST_MAIN(FrameworkTest)
#include "framework_test.moc"
