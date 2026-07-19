#include <QTest>
#include <QApplication>
#include <QSignalSpy>
#include "gui/framework/ThemeManager.hpp"
#include "gui/framework/SettingsManager.hpp"
#include "gui/framework/NotificationCenter.hpp"
#include "gui/framework/MainWindow.hpp"
#include "gui/framework/ApplicationController.hpp"
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
#include "gui/diagnostics/DiagnosticsWidget.hpp"

class IntegrationTest : public QObject {
    Q_OBJECT
private slots:
    void testThemeSwitch();
    void testNotificationFlow();
    void testSettingsPersistence();
};

void IntegrationTest::testThemeSwitch()
{
    ThemeManager tm;
    SettingsManager sm;

    tm.applyTheme(ThemeManager::Dark);
    QCOMPARE(tm.currentTheme(), ThemeManager::Dark);

    sm.setTheme("Light");
    tm.applyTheme("Light");
    QCOMPARE(tm.currentTheme(), ThemeManager::Light);

    tm.applyTheme(ThemeManager::Classic);
    QCOMPARE(tm.currentTheme(), ThemeManager::Classic);
}

void IntegrationTest::testNotificationFlow()
{
    NotificationCenter nc;
    QSignalSpy spy(&nc, &NotificationCenter::notificationPosted);

    QString id = nc.post("Test", "Integration test", NotificationCenter::Info);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(nc.unreadCount(), 1);

    nc.dismiss(id);
    QCOMPARE(nc.activeNotifications().size(), 0);

    nc.clear();
    QCOMPARE(nc.count(), 0);
}

void IntegrationTest::testSettingsPersistence()
{
    SettingsManager sm;

    sm.setTheme("Dark");
    sm.setAccentColor(QColor("#ff6600"));
    sm.setDeveloperMode(true);

    QCOMPARE(sm.theme(), QString("Dark"));
    QCOMPARE(sm.accentColor(), QColor("#ff6600"));
    QVERIFY(sm.developerMode());

    sm.sync();
}

QTEST_MAIN(IntegrationTest)
#include "integration_test.moc"
