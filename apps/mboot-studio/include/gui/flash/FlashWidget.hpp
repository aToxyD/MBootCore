#pragma once

#include "gui/runtime/RuntimeModels.hpp"

#include <QWidget>
#include <QSplitter>
#include <memory>

namespace gui::runtime {
class RuntimeBridge;
}

class FlashPlanViewer;
class PartitionSelection;
class ProgressWindow;
class FlashStatistics;
class QPushButton;
class QLabel;

class FlashWidget : public QWidget {
    Q_OBJECT
public:
    explicit FlashWidget(QWidget *parent = nullptr);

    void setRuntimeBridge(gui::runtime::RuntimeBridge *bridge);
    void loadPlan(const QString &path);
    void startFlash();
    void stopFlash();
    void pauseFlash();
    void resumeFlash();

signals:
    void flashStarted();
    void flashCompleted(bool success);
    void flashProgress(int percent);
    void flashError(const QString &error);

private slots:
    void onFlashOperationChanged(const gui::runtime::FlashOperationView &op);
    void onFlashProgressChanged(const gui::runtime::FlashProgressView &progress);
    void onFlashCompleted(const gui::runtime::FlashResultView &result);
    void onFlashCancelled();
    void onFlashFailed(const QString &error);
    void onPackageLoaded(const QString &path, bool success);

private:
    void setupUi();
    void updateButtonStates(gui::runtime::FlashStatus status);
    gui::runtime::RuntimeBridge *m_bridge{nullptr};
    FlashPlanViewer *m_planViewer;
    PartitionSelection *m_partitionSelection;
    ProgressWindow *m_progressWindow;
    FlashStatistics *m_statistics;
    QPushButton *m_startBtn, *m_stopBtn, *m_pauseBtn, *m_resumeBtn;
    QLabel *m_statusLabel;
};
