#include "gui/workflow/WorkflowDesigner.hpp"
#include "gui/workflow/WorkflowGraph.hpp"
#include "gui/workflow/StepInspector.hpp"
#include "gui/workflow/RecoveryViewer.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QComboBox>
#include <QFileDialog>
#include <QMessageBox>

WorkflowDesigner::WorkflowDesigner(QWidget *parent)
    : QWidget(parent)
    , m_splitter(new QSplitter(Qt::Horizontal, this))
    , m_graph(new WorkflowGraph(this))
    , m_stepInspector(new StepInspector(this))
    , m_recoveryViewer(new RecoveryViewer(this))
    , m_executeBtn(new QPushButton(tr("Execute"), this))
    , m_stopBtn(new QPushButton(tr("Stop"), this))
    , m_debugBtn(new QPushButton(tr("Debug"), this))
    , m_loadBtn(new QPushButton(tr("Load"), this))
    , m_workflowCombo(new QComboBox(this))
{
    setupUi();
}

void WorkflowDesigner::setupUi()
{
    auto *mainLayout = new QVBoxLayout(this);

    m_loadBtn->setAccessibleName("Load Workflow");
    m_loadBtn->setAccessibleDescription("Load a workflow definition from a file");
    m_loadBtn->setToolTip("Load a workflow definition file");
    m_workflowCombo->setAccessibleName("Workflow Selection");
    m_workflowCombo->setAccessibleDescription("Select a loaded workflow to execute");
    m_workflowCombo->setToolTip("Choose a workflow from the loaded definitions");
    m_executeBtn->setAccessibleName("Execute Workflow");
    m_executeBtn->setAccessibleDescription("Execute the selected workflow");
    m_executeBtn->setToolTip("Run the selected workflow");
    m_executeBtn->setDefault(true);
    m_stopBtn->setAccessibleName("Stop Workflow");
    m_stopBtn->setAccessibleDescription("Stop the currently executing workflow");
    m_stopBtn->setToolTip("Stop the running workflow");
    m_debugBtn->setAccessibleName("Debug Workflow");
    m_debugBtn->setAccessibleDescription("Execute the workflow in debug mode with step-by-step control");
    m_debugBtn->setToolTip("Debug the workflow step by step");

    m_graph->setAccessibleName("Workflow Graph");
    m_graph->setAccessibleDescription("Visual representation of the workflow steps and connections");
    m_stepInspector->setAccessibleName("Step Inspector");
    m_stepInspector->setAccessibleDescription("Inspect and edit workflow step properties");
    m_recoveryViewer->setAccessibleName("Recovery Viewer");
    m_recoveryViewer->setAccessibleDescription("View recovery and error handling configuration");

    auto *toolbar = new QHBoxLayout();
    toolbar->addWidget(m_loadBtn);
    toolbar->addWidget(m_workflowCombo);
    toolbar->addWidget(m_executeBtn);
    toolbar->addWidget(m_stopBtn);
    toolbar->addWidget(m_debugBtn);
    mainLayout->addLayout(toolbar);

    m_splitter->addWidget(m_graph);
    m_splitter->addWidget(m_stepInspector);
    m_splitter->addWidget(m_recoveryViewer);
    mainLayout->addWidget(m_splitter);

    m_stopBtn->setEnabled(false);

    connect(m_loadBtn, &QPushButton::clicked, this, [this]() {
        QString path = QFileDialog::getOpenFileName(this, tr("Open Workflow"));
        if (!path.isEmpty())
            loadWorkflow(path);
    });

    connect(m_executeBtn, &QPushButton::clicked, this, &WorkflowDesigner::executeWorkflow);
    connect(m_stopBtn, &QPushButton::clicked, this, &WorkflowDesigner::stopWorkflow);
    connect(m_debugBtn, &QPushButton::clicked, this, &WorkflowDesigner::debugWorkflow);
}

void WorkflowDesigner::loadWorkflow(const QString &path)
{
    Q_UNUSED(path)
    m_workflowCombo->addItem(path);
    emit workflowCompleted(true);
}

void WorkflowDesigner::executeWorkflow()
{
    m_executeBtn->setEnabled(false);
    m_stopBtn->setEnabled(true);
    emit workflowStarted();
}

void WorkflowDesigner::stopWorkflow()
{
    m_stopBtn->setEnabled(false);
    m_executeBtn->setEnabled(true);
    emit workflowCompleted(false);
}

void WorkflowDesigner::debugWorkflow()
{
    QMessageBox::information(this, tr("Debug"), tr("Debug mode activated"));
}
