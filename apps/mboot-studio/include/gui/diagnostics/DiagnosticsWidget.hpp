#pragma once

#include "gui/runtime/RuntimeModels.hpp"

#include <QWidget>
#include <QTabWidget>
#include <memory>

namespace gui::runtime {
class RuntimeBridge;
}

class QPushButton;
class QTextEdit;
class QLabel;
class QTableWidget;

class DiagnosticsWidget : public QWidget {
    Q_OBJECT
public:
    explicit DiagnosticsWidget(QWidget *parent = nullptr);

    void setRuntimeBridge(gui::runtime::RuntimeBridge *bridge);
    void runAll();
    void exportReport();

    static QString buildReport(const gui::runtime::RuntimeDiagnosticsView &diagnostics);
    static bool saveReport(const QString &path, const QString &report);

private slots:
    void onDiagnosticsUpdated(const gui::runtime::RuntimeDiagnosticsView &diagnostics);
    void onHealthChanged(const gui::runtime::RuntimeHealthView &health);

private:
    void setupUi();
    void updateRuntimeTab(const gui::runtime::RuntimeDiagnosticsView &diagnostics);
    void updateTransportTab(const gui::runtime::RuntimeDiagnosticsView &diagnostics);
    void updatePluginTab(const gui::runtime::RuntimeDiagnosticsView &diagnostics);
    void updateEnvironmentTab(const gui::runtime::RuntimeDiagnosticsView &diagnostics);
    void appendOutput(const QString &text);

    gui::runtime::RuntimeBridge *m_bridge{nullptr};
    QTabWidget *m_tabs;
    QTableWidget *m_runtimeTable;
    QTableWidget *m_transportTable;
    QTableWidget *m_pluginTable;
    QTableWidget *m_environmentTable;
    QTextEdit *m_outputView;
    QPushButton *m_runBtn, *m_exportBtn, *m_refreshBtn;
    gui::runtime::RuntimeDiagnosticsView m_lastDiagnostics;
};
