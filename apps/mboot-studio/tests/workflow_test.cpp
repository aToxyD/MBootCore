#include <QTest>
#include <QApplication>
#include <QSignalSpy>
#include "gui/workflow/WorkflowDesigner.hpp"
#include "gui/workflow/WorkflowGraph.hpp"
#include "gui/workflow/StepInspector.hpp"
#include "gui/workflow/RecoveryViewer.hpp"

class WorkflowTest : public QObject {
    Q_OBJECT
private slots:
    void testWorkflowDesigner();
    void testWorkflowGraph();
    void testStepInspector();
    void testRecoveryViewer();
};

void WorkflowTest::testWorkflowDesigner()
{
    WorkflowDesigner w;
    QVERIFY(w.width() > 0);
}

void WorkflowTest::testWorkflowGraph()
{
    WorkflowGraph graph;
    QVariantList steps;
    QVariantMap s; s["id"] = "step1"; s["name"] = "Step 1";
    steps << s;
    graph.setSteps(steps);
    graph.highlightStep("step1");
    graph.clear();
}

void WorkflowTest::testStepInspector()
{
    StepInspector inspector;
    QVariantMap step;
    step["name"] = "Flash";
    step["type"] = "flash";
    step["status"] = "completed";
    inspector.inspectStep(step);
    inspector.clear();
}

void WorkflowTest::testRecoveryViewer()
{
    RecoveryViewer viewer;
    QVariantList plan;
    QVariantMap p; p["step"] = "flash"; p["action"] = "retry";
    plan << p;
    viewer.setRecoveryPlan(plan);
    viewer.clear();
}

QTEST_MAIN(WorkflowTest)
#include "workflow_test.moc"
