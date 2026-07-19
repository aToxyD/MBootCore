#include "gui/job/ProgressAggregatorWidget.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>

ProgressAggregatorWidget::ProgressAggregatorWidget(QWidget *parent)
    : QWidget(parent)
    , m_overallBar(new QProgressBar(this))
    , m_activeLabel(new QLabel(tr("Active: 0"), this))
    , m_completedLabel(new QLabel(tr("Completed: 0"), this))
    , m_failedLabel(new QLabel(tr("Failed: 0"), this))
{
    setupUi();
}

void ProgressAggregatorWidget::setupUi()
{
    auto *layout = new QVBoxLayout(this);

    layout->addWidget(new QLabel(tr("Overall Progress"), this));
    m_overallBar->setRange(0, 100);
    layout->addWidget(m_overallBar);

    auto *statsLayout = new QHBoxLayout();
    statsLayout->addWidget(m_activeLabel);
    statsLayout->addWidget(m_completedLabel);
    statsLayout->addWidget(m_failedLabel);
    layout->addLayout(statsLayout);
}

void ProgressAggregatorWidget::setOverallProgress(int percent)
{
    m_overallBar->setValue(percent);
}

void ProgressAggregatorWidget::setActiveJobs(int count)
{
    m_activeLabel->setText(tr("Active: %1").arg(count));
}

void ProgressAggregatorWidget::setCompletedJobs(int count)
{
    m_completedLabel->setText(tr("Completed: %1").arg(count));
}

void ProgressAggregatorWidget::setFailedJobs(int count)
{
    m_failedLabel->setText(tr("Failed: %1").arg(count));
}

void ProgressAggregatorWidget::reset()
{
    m_overallBar->setValue(0);
    setActiveJobs(0);
    setCompletedJobs(0);
    setFailedJobs(0);
}
