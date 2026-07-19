#include <QTest>
#include <QApplication>
#include <QSignalSpy>
#include "gui/flash/FlashWidget.hpp"
#include "gui/flash/FlashPlanViewer.hpp"
#include "gui/flash/PartitionSelection.hpp"
#include "gui/flash/ProgressWindow.hpp"
#include "gui/flash/FlashStatistics.hpp"

class FlashTest : public QObject {
    Q_OBJECT
private slots:
    void testFlashWidget();
    void testFlashPlanViewer();
    void testPartitionSelection();
    void testProgressWindow();
    void testFlashStatistics();
};

void FlashTest::testFlashWidget()
{
    FlashWidget widget;
    QVERIFY(widget.width() > 0);
}

void FlashTest::testFlashPlanViewer()
{
    FlashPlanViewer viewer;
    QVariantMap plan;
    plan["name"] = "Test Plan";
    viewer.setPlan(plan);
    viewer.clear();
}

void FlashTest::testPartitionSelection()
{
    PartitionSelection sel;
    QVariantList parts;
    QVariantMap p; p["name"] = "boot";
    parts << p;
    sel.setPartitions(parts);
    QCOMPARE(sel.selectedPartitions().size(), 1);
    QCOMPARE(sel.selectedPartitions().first(), QString("boot"));
    sel.selectAll(false);
    QVERIFY(sel.selectedPartitions().isEmpty());
    sel.selectAll(true);
    QCOMPARE(sel.selectedPartitions().size(), 1);
}

void FlashTest::testProgressWindow()
{
    ProgressWindow pw;
    pw.setProgress(50);
    pw.setStatus("Writing...");
    pw.setOperation("flash");
    pw.setEstimatedTime(30000);
    pw.reset();
}

void FlashTest::testFlashStatistics()
{
    FlashStatistics stats;
    stats.setBytesTransferred(1048576);
    stats.setTransferSpeed(50000.0);
    stats.setRetryCount(2);
    stats.setErrorCount(0);
    stats.setWarningCount(1);
    stats.setElapsed(15000);
    stats.reset();
}

QTEST_MAIN(FlashTest)
#include "flash_test.moc"
