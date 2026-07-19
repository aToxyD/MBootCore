#pragma once

#include "gui/runtime/RuntimeModels.hpp"

#include <QWidget>
#include <QTimer>
#include <memory>

class QTreeView;
class QListView;
class QLineEdit;
class QPushButton;
class QComboBox;
class QLabel;
class QStackedWidget;
class DeviceListModel;
class DeviceTreeModel;
class DeviceFilterProxyModel;
class DeviceDetailsWidget;

namespace gui::runtime {
class RuntimeBridge;
}

class DeviceDiscoveryWidget : public QWidget {
    Q_OBJECT
public:
    explicit DeviceDiscoveryWidget(QWidget *parent = nullptr);
    ~DeviceDiscoveryWidget() override;

    void setRuntimeBridge(gui::runtime::RuntimeBridge *bridge);
    void startDiscovery();
    void stopDiscovery();
    void setAutoRefresh(bool enabled);
    bool isDiscovering() const { return m_discovering; }
    void setFilterText(const QString &text);
    void setViewMode(int mode);

signals:
    void deviceSelected(const QString &deviceId);
    void deviceConnected(const QString &deviceId);
    void discoveryStarted();
    void discoveryStopped();
    void discoveryError(const QString &error);

private slots:
    void onRefresh();
    void onSearchChanged(const QString &text);
    void onDeviceActivated(const QModelIndex &index);
    void onViewModeChanged(int mode);
    void onDeviceListChanged(const std::vector<gui::runtime::DeviceInfoView>& devices);
    void onDiscoveryFailed(const QString& error);

private:
    void setupUi();
    void setupModels();
    void setupToolBar();
    void connectRuntimeBridge();

    QTreeView *m_treeView;
    QListView *m_listView;
    QStackedWidget *m_viewStack;
    QLineEdit *m_searchBox;
    QPushButton *m_refreshBtn;
    QPushButton *m_autoRefreshBtn;
    QComboBox *m_viewModeCombo;
    QLabel *m_statusLabel;

    DeviceListModel *m_listModel;
    DeviceTreeModel *m_treeModel;
    DeviceFilterProxyModel *m_proxyModel;
    DeviceDetailsWidget *m_detailsWidget;

    QTimer *m_refreshTimer;
    bool m_discovering{false};
    gui::runtime::RuntimeBridge *m_runtimeBridge{nullptr};
};
