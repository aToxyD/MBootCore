#pragma once

#include <QWidget>
#include <QGraphicsView>
#include <QSplitter>
#include <memory>

class WorkflowGraph;
class StepInspector;
class RecoveryViewer;
class QPushButton;
class QComboBox;

class WorkflowDesigner : public QWidget {
    Q_OBJECT
public:
    explicit WorkflowDesigner(QWidget *parent = nullptr);
    void loadWorkflow(const QString &path);
    void executeWorkflow();
    void stopWorkflow();
    void debugWorkflow();

signals:
    void workflowStarted();
    void workflowCompleted(bool success);
    void workflowError(const QString &error);

private:
    void setupUi();
    QSplitter *m_splitter;
    WorkflowGraph *m_graph;
    StepInspector *m_stepInspector;
    RecoveryViewer *m_recoveryViewer;
    QPushButton *m_executeBtn, *m_stopBtn, *m_debugBtn, *m_loadBtn;
    QComboBox *m_workflowCombo;
};
