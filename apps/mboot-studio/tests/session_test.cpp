#include <QTest>
#include <QApplication>
#include <QSignalSpy>
#include "gui/session/SessionWidget.hpp"
#include "gui/session/SessionMonitor.hpp"
#include "gui/session/SessionStatistics.hpp"

class SessionTest : public QObject {
    Q_OBJECT
private slots:
    void testSessionWidget();
    void testSessionMonitor();
    void testSessionStatistics();
};

void SessionTest::testSessionWidget()
{
    SessionWidget widget;
    QVERIFY(widget.width() > 0);
}

void SessionTest::testSessionMonitor()
{
    SessionMonitor monitor;
    QSignalSpy spy(&monitor, &SessionMonitor::logEntryAdded);
    monitor.appendLog("Test log message", "info");
    QCOMPARE(spy.count(), 1);
    monitor.appendLog("Warning message", "warning");
    QCOMPARE(spy.count(), 2);
    monitor.appendLog("Error message", "error");
    QCOMPARE(spy.count(), 3);
    monitor.clear();
    monitor.setAutoScroll(false);
}

void SessionTest::testSessionStatistics()
{
    SessionStatistics stats;
    stats.setBytesRead(1024);
    stats.setBytesWritten(2048);
    stats.setTransferSpeed(500.0);
    stats.setElapsed(10000);
    stats.setErrors(0);
    stats.setWarnings(2);
    stats.reset();
}

QTEST_MAIN(SessionTest)
#include "session_test.moc"
