#include <QTest>
#include <QApplication>
#include <QSignalSpy>
#include "gui/plugin/PluginManagerWidget.hpp"

class PluginTest : public QObject {
    Q_OBJECT
private slots:
    void testPluginManagerWidget();
};

void PluginTest::testPluginManagerWidget()
{
    PluginManagerWidget w;
    QVERIFY(w.width() > 0);
    w.loadPlugins();
    w.enablePlugin("test-plugin");
    w.disablePlugin("test-plugin");
    w.unloadPlugin("test-plugin");
}

QTEST_MAIN(PluginTest)
#include "plugin_test.moc"
