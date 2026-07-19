#include <QTest>
#include <QApplication>
#include "gui/devtools/DeveloperToolsWidget.hpp"

class DevToolsTest : public QObject {
    Q_OBJECT
private slots:
    void testDeveloperToolsWidget();
};

void DevToolsTest::testDeveloperToolsWidget()
{
    DeveloperToolsWidget w;
    QVERIFY(w.width() > 0);
}

QTEST_MAIN(DevToolsTest)
#include "devtools_test.moc"
