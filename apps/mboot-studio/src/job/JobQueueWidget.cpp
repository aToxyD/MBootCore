#include "gui/job/JobQueueWidget.hpp"
#include "gui/job/JobHistoryWidget.hpp"
#include "gui/job/SchedulerView.hpp"
#include "gui/job/ProgressAggregatorWidget.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QHeaderView>
#include <QDateTime>

JobQueueWidget::JobQueueWidget(QWidget *parent)
    : QWidget(parent)
    , m_queueTable(new QTableWidget(this))
    , m_splitter(new QSplitter(Qt::Vertical, this))
    , m_historyWidget(new JobHistoryWidget(this))
    , m_schedulerView(new SchedulerView(this))
    , m_progressWidget(new ProgressAggregatorWidget(this))
    , m_cancelBtn(new QPushButton(tr("Cancel"), this))
    , m_pauseBtn(new QPushButton(tr("Pause"), this))
    , m_resumeBtn(new QPushButton(tr("Resume"), this))
{
    setupUi();
}

void JobQueueWidget::setupUi()
{
    auto *mainLayout = new QVBoxLayout(this);

    m_progressWidget->setAccessibleName("Progress Overview");
    m_progressWidget->setAccessibleDescription("Aggregated progress of all active jobs");

    mainLayout->addWidget(m_progressWidget);

    m_queueTable->setAccessibleName("Job Queue");
    m_queueTable->setAccessibleDescription("List of queued jobs with ID, type, status, and progress");
    m_queueTable->setColumnCount(4);
    m_queueTable->setHorizontalHeaderLabels({tr("ID"), tr("Type"), tr("Status"), tr("Progress")});
    m_queueTable->horizontalHeader()->setStretchLastSection(true);
    m_queueTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    mainLayout->addWidget(m_queueTable);

    m_cancelBtn->setAccessibleName("Cancel Job");
    m_cancelBtn->setAccessibleDescription("Cancel the selected queued job");
    m_cancelBtn->setToolTip("Cancel the selected job");
    m_pauseBtn->setAccessibleName("Pause Scheduler");
    m_pauseBtn->setAccessibleDescription("Pause the job scheduler");
    m_pauseBtn->setToolTip("Pause job execution");
    m_resumeBtn->setAccessibleName("Resume Scheduler");
    m_resumeBtn->setAccessibleDescription("Resume the job scheduler");
    m_resumeBtn->setToolTip("Resume paused job execution");

    auto *btnLayout = new QHBoxLayout();
    btnLayout->addWidget(m_cancelBtn);
    btnLayout->addWidget(m_pauseBtn);
    btnLayout->addWidget(m_resumeBtn);
    mainLayout->addLayout(btnLayout);

    QWidget::setTabOrder(m_cancelBtn, m_pauseBtn);
    QWidget::setTabOrder(m_pauseBtn, m_resumeBtn);
    QWidget::setTabOrder(m_resumeBtn, m_queueTable);

    m_schedulerView->setAccessibleName("Scheduler View");
    m_schedulerView->setAccessibleDescription("View and manage the job scheduler");
    m_historyWidget->setAccessibleName("Job History");
    m_historyWidget->setAccessibleDescription("History of completed jobs");

    m_splitter->addWidget(m_schedulerView);
    m_splitter->addWidget(m_historyWidget);
    mainLayout->addWidget(m_splitter);

    m_resumeBtn->setEnabled(false);

    connect(m_cancelBtn, &QPushButton::clicked, this, [this]() {
        auto selected = m_queueTable->selectedItems();
        if (!selected.isEmpty())
            cancelJob(selected.first()->text());
    });
    connect(m_pauseBtn, &QPushButton::clicked, this, &JobQueueWidget::pauseScheduler);
    connect(m_resumeBtn, &QPushButton::clicked, this, &JobQueueWidget::resumeScheduler);
}

void JobQueueWidget::submitJob(const QString &type, const QVariantMap &params)
{
    Q_UNUSED(params)
    QString jobId = QString("job_%1").arg(QDateTime::currentMSecsSinceEpoch());
    int row = m_queueTable->rowCount();
    m_queueTable->insertRow(row);
    m_queueTable->setItem(row, 0, new QTableWidgetItem(jobId));
    m_queueTable->setItem(row, 1, new QTableWidgetItem(type));
    m_queueTable->setItem(row, 2, new QTableWidgetItem(tr("Queued")));
    m_queueTable->setItem(row, 3, new QTableWidgetItem("0%"));
    emit jobSubmitted(jobId);
}

void JobQueueWidget::cancelJob(const QString &jobId)
{
    for (int i = 0; i < m_queueTable->rowCount(); ++i) {
        if (m_queueTable->item(i, 0)->text() == jobId) {
            m_queueTable->item(i, 2)->setText(tr("Cancelled"));
            emit jobCompleted(jobId, false);
            break;
        }
    }
}

void JobQueueWidget::pauseScheduler()
{
    m_pauseBtn->setEnabled(false);
    m_resumeBtn->setEnabled(true);
}

void JobQueueWidget::resumeScheduler()
{
    m_resumeBtn->setEnabled(false);
    m_pauseBtn->setEnabled(true);
}
