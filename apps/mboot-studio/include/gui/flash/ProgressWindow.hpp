#pragma once

#include <QWidget>
#include <QProgressBar>
#include <QLabel>
#include <memory>

class ProgressWindow : public QWidget {
    Q_OBJECT
public:
    explicit ProgressWindow(QWidget *parent = nullptr);
    void setProgress(int percent);
    void setStatus(const QString &status);
    void setOperation(const QString &op);
    void setEstimatedTime(qint64 ms);
    void reset();

private:
    void setupUi();
    QProgressBar *m_progressBar;
    QLabel *m_statusLabel, *m_operationLabel, *m_etaLabel;
};
