#pragma once

#include <QWidget>
#include <QTableWidget>
#include <QSplitter>
#include <memory>

class JobHistoryWidget;
class SchedulerView;
class ProgressAggregatorWidget;
class QPushButton;

class JobQueueWidget : public QWidget {
    Q_OBJECT
public:
    explicit JobQueueWidget(QWidget *parent = nullptr);
    void submitJob(const QString &type, const QVariantMap &params);
    void cancelJob(const QString &jobId);
    void pauseScheduler();
    void resumeScheduler();

signals:
    void jobSubmitted(const QString &jobId);
    void jobCompleted(const QString &jobId, bool success);

private:
    void setupUi();
    QTableWidget *m_queueTable;
    QSplitter *m_splitter;
    JobHistoryWidget *m_historyWidget;
    SchedulerView *m_schedulerView;
    ProgressAggregatorWidget *m_progressWidget;
    QPushButton *m_cancelBtn, *m_pauseBtn, *m_resumeBtn;
};
