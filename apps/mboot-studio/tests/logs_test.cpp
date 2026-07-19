#include <QTest>
#include <QApplication>
#include "gui/logs/LogViewerWidget.hpp"

class LogsTest : public QObject {
    Q_OBJECT
private slots:
    void testLogViewerWidget();
};

void LogsTest::testLogViewerWidget()
{
    LogViewerWidget w;
    w.appendLog("Test message", "info");
    w.appendLog("Warning", "warning");
    w.appendLog("Error", "error");
    w.setFilter("error");

    QString txt = w.exportText();
    QVERIFY(!txt.isEmpty());

    QString csv = w.exportCsv();
    QVERIFY(!csv.isEmpty());

    QString json = w.exportJson();
    QVERIFY(!json.isEmpty());

    w.clear();
}

QTEST_MAIN(LogsTest)
#include "logs_test.moc"
