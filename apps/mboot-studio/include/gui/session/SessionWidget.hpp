#pragma once

#include "gui/runtime/RuntimeModels.hpp"

#include <QWidget>
#include <QTabWidget>
#include <memory>

class SessionMonitor;
class SessionStatistics;
class QPushButton;
class QLabel;

namespace gui::runtime {
class RuntimeBridge;
}

class SessionWidget : public QWidget {
    Q_OBJECT
public:
    explicit SessionWidget(QWidget *parent = nullptr);

    void setRuntimeBridge(gui::runtime::RuntimeBridge *bridge);

signals:
    void connectRequested();
    void disconnectRequested();
    void pauseRequested();
    void resumeRequested();
    void cancelRequested();

private slots:
    void onSessionStatusChanged(const gui::runtime::SessionInfoView& session);
    void onConnectionSucceeded(const QString& connectionPath);
    void onDisconnected();

private:
    void setupUi();
    void connectRuntimeBridge();

    QTabWidget *m_tabs;
    SessionMonitor *m_monitor;
    SessionStatistics *m_stats;
    QPushButton *m_connectBtn, *m_disconnectBtn, *m_pauseBtn, *m_resumeBtn, *m_cancelBtn;
    QLabel *m_statusLabel;
    gui::runtime::RuntimeBridge *m_runtimeBridge{nullptr};
};
