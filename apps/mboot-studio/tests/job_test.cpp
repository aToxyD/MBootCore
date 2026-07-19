#include <QTest>
#include <QApplication>
#include <QSignalSpy>
#include "gui/job/JobQueueWidget.hpp"
#include "gui/job/JobHistoryWidget.hpp"
#include "gui/job/SchedulerView.hpp"
#include "gui/job/ProgressAggregatorWidget.hpp"
#include "gui/job/RollbackViewer.hpp"
#include "gui/job/RecoveryPolicyEditor.hpp"

class JobTest : public QObject {
    Q_OBJECT
private slots:
    void testJobQueueWidget();
    void testJobHistoryWidget();
    void testSchedulerView();
    void testProgressAggregatorWidget();
    void testRollbackViewer();
    void testRecoveryPolicyEditor();
};

void JobTest::testJobQueueWidget()
{
    JobQueueWidget w;
    QVERIFY(w.width() > 0);
}

void JobTest::testJobHistoryWidget()
{
    JobHistoryWidget hw;
    QVariantMap e;
    e["id"] = "job-1";
    e["status"] = "completed";
    hw.addEntry(e);
    hw.setFilter("completed");
    hw.clear();
}

void JobTest::testSchedulerView()
{
    SchedulerView sv;
    QVariantList schedule;
    QVariantMap s; s["job"] = "job-1"; s["time"] = "now";
    schedule << s;
    sv.setSchedule(schedule);
    sv.clear();
}

void JobTest::testProgressAggregatorWidget()
{
    ProgressAggregatorWidget pw;
    pw.setOverallProgress(50);
    pw.setActiveJobs(3);
    pw.setCompletedJobs(10);
    pw.setFailedJobs(1);
    pw.reset();
}

void JobTest::testRollbackViewer()
{
    RollbackViewer rv;
    QVariantList plan;
    QVariantMap p; p["step"] = "flash"; p["rollback"] = "erase";
    plan << p;
    rv.setRollbackPlan(plan);
    rv.clear();
}

void JobTest::testRecoveryPolicyEditor()
{
    RecoveryPolicyEditor editor;
    QVariantMap policy;
    policy["maxRetries"] = 3;
    policy["autoRollback"] = true;
    editor.setPolicy(policy);
    auto result = editor.policy();
    QCOMPARE(result["maxRetries"].toInt(), 3);
}

QTEST_MAIN(JobTest)
#include "job_test.moc"
