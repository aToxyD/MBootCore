#pragma once

#include "gui/runtime/RuntimeModels.hpp"

#include <QWidget>
#include <QTableWidget>
#include <QPushButton>
#include <QLabel>
#include <memory>

namespace gui::runtime {
class RuntimeBridge;
}

class PluginManagerWidget : public QWidget {
    Q_OBJECT
public:
    explicit PluginManagerWidget(QWidget *parent = nullptr);

    void setRuntimeBridge(gui::runtime::RuntimeBridge *bridge);
    void loadPlugins();
    void unloadPlugin(const QString &id);
    void enablePlugin(const QString &id);
    void disablePlugin(const QString &id);

signals:
    void pluginLoaded(const QString &id);
    void pluginUnloaded(const QString &id);
    void pluginStateChanged(const QString &id, bool enabled);

private slots:
    void onPluginListChanged();
    void onPluginLoaded(const QString &name);
    void onPluginUnloaded(const QString &name);
    void onCapabilitiesChanged();

private:
    void setupUi();
    void updatePluginTable();

    gui::runtime::RuntimeBridge *m_bridge{nullptr};
    QTableWidget *m_table;
    QTableWidget *m_capabilitiesTable;
    QPushButton *m_loadBtn, *m_unloadBtn, *m_enableBtn, *m_disableBtn, *m_refreshBtn;
    QLabel *m_statusLabel;
    std::vector<gui::runtime::PluginInfoView> m_plugins;
    std::vector<gui::runtime::CapabilityView> m_capabilities;
};
