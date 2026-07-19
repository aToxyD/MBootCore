#pragma once

#include <QWidget>
#include <QProgressBar>
#include <QLabel>
#include <memory>

class ProgressAggregatorWidget : public QWidget {
    Q_OBJECT
public:
    explicit ProgressAggregatorWidget(QWidget *parent = nullptr);
    void setOverallProgress(int percent);
    void setActiveJobs(int count);
    void setCompletedJobs(int count);
    void setFailedJobs(int count);
    void reset();

private:
    void setupUi();
    QProgressBar *m_overallBar;
    QLabel *m_activeLabel, *m_completedLabel, *m_failedLabel;
};
