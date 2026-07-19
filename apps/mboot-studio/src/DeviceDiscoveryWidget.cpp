#include "gui/discovery/DeviceDiscoveryWidget.hpp"
#include "gui/discovery/DeviceListModel.hpp"
#include "gui/discovery/DeviceTreeModel.hpp"
#include "gui/discovery/DeviceFilterProxyModel.hpp"
#include "gui/discovery/DeviceDetailsWidget.hpp"
#include "gui/runtime/RuntimeBridge.hpp"
#include "gui/runtime/RuntimeModels.hpp"
#include "gui/runtime/RuntimeEventDispatcher.hpp"
#include "gui/runtime/RuntimeErrorMapper.hpp"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QStackedWidget>
#include <QTreeView>
#include <QListView>
#include <QLineEdit>
#include <QPushButton>
#include <QComboBox>
#include <QLabel>
#include <QTimer>
#include <QHeaderView>
#include <QApplication>

DeviceDiscoveryWidget::DeviceDiscoveryWidget(QWidget *parent)
    : QWidget(parent)
    , m_treeView(new QTreeView)
    , m_listView(new QListView)
    , m_viewStack(new QStackedWidget)
    , m_searchBox(new QLineEdit)
    , m_refreshBtn(new QPushButton)
    , m_autoRefreshBtn(new QPushButton)
    , m_viewModeCombo(new QComboBox)
    , m_statusLabel(new QLabel)
    , m_listModel(new DeviceListModel(this))
    , m_treeModel(new DeviceTreeModel(this))
    , m_proxyModel(new DeviceFilterProxyModel(this))
    , m_detailsWidget(new DeviceDetailsWidget)
    , m_refreshTimer(new QTimer(this))
{
    setupUi();
    setupModels();
    setupToolBar();

    connect(m_refreshBtn, &QPushButton::clicked, this, &DeviceDiscoveryWidget::onRefresh);
    connect(m_autoRefreshBtn, &QPushButton::clicked, this, [this]() {
        setAutoRefresh(!m_refreshTimer->isActive());
    });
    connect(m_refreshTimer, &QTimer::timeout, this, &DeviceDiscoveryWidget::onRefresh);
    connect(m_searchBox, &QLineEdit::textChanged, this, &DeviceDiscoveryWidget::onSearchChanged);
    connect(m_viewModeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &DeviceDiscoveryWidget::onViewModeChanged);

    connect(m_treeView, &QTreeView::activated, this, &DeviceDiscoveryWidget::onDeviceActivated);
    connect(m_listView, &QListView::activated, this, &DeviceDiscoveryWidget::onDeviceActivated);

    connect(m_detailsWidget, &DeviceDetailsWidget::connectRequested,
            this, &DeviceDiscoveryWidget::deviceConnected);
}

DeviceDiscoveryWidget::~DeviceDiscoveryWidget() = default;

void DeviceDiscoveryWidget::setRuntimeBridge(gui::runtime::RuntimeBridge *bridge)
{
    if (m_runtimeBridge == bridge) return;

    if (m_runtimeBridge) {
        auto *ed = m_runtimeBridge->eventDispatcher();
        disconnect(ed, &gui::runtime::RuntimeEventDispatcher::deviceListChanged,
                   this, &DeviceDiscoveryWidget::onDeviceListChanged);
        disconnect(ed, &gui::runtime::RuntimeEventDispatcher::errorOccurred,
                   this, nullptr);
    }

    m_runtimeBridge = bridge;

    if (m_runtimeBridge) {
        connectRuntimeBridge();
    }
}

void DeviceDiscoveryWidget::connectRuntimeBridge()
{
    if (!m_runtimeBridge) return;

    auto *ed = m_runtimeBridge->eventDispatcher();
    connect(ed, &gui::runtime::RuntimeEventDispatcher::deviceListChanged,
            this, &DeviceDiscoveryWidget::onDeviceListChanged);
    connect(ed, &gui::runtime::RuntimeEventDispatcher::errorOccurred,
            this, [this](const gui::runtime::RuntimeError& error) {
                if (error.coreCode == mbootcore::ErrorCode::DeviceNotFound ||
                    error.coreCode == mbootcore::ErrorCode::EnumerationFailed) {
                    onDiscoveryFailed(QString::fromStdString(error.message));
                }
            });
}

void DeviceDiscoveryWidget::setupUi()
{
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    auto *splitter = new QSplitter(Qt::Horizontal);

    m_viewStack->addWidget(m_treeView);
    m_viewStack->addWidget(m_listView);
    m_viewStack->setCurrentIndex(0);

    m_treeView->setAccessibleName("Device Tree");
    m_treeView->setAccessibleDescription("View discovered devices in a tree structure");
    m_listView->setAccessibleName("Device List");
    m_listView->setAccessibleDescription("View discovered devices in a list layout");
    m_viewStack->setAccessibleName("Device View");
    m_viewStack->setAccessibleDescription("Switch between tree and list device views");
    m_detailsWidget->setAccessibleName("Device Details");
    m_detailsWidget->setAccessibleDescription("View detailed information about the selected device");

    splitter->addWidget(m_viewStack);
    splitter->addWidget(m_detailsWidget);
    splitter->setStretchFactor(0, 3);
    splitter->setStretchFactor(1, 1);

    mainLayout->addWidget(splitter);

    m_statusLabel->setFrameStyle(QFrame::StyledPanel);
    m_statusLabel->setAccessibleName("Discovery Status");
    m_statusLabel->setAccessibleDescription("Current device discovery status");
    mainLayout->addWidget(m_statusLabel);
}

void DeviceDiscoveryWidget::setupModels()
{
    m_proxyModel->setSourceModel(m_listModel);
    m_treeView->setModel(m_treeModel);
    m_listView->setModel(m_proxyModel);

    m_treeView->setAnimated(true);
    m_treeView->setIndentation(20);
    m_treeView->setExpandsOnDoubleClick(true);
    m_treeView->header()->setStretchLastSection(true);

    m_listView->setViewMode(QListView::IconMode);
    m_listView->setIconSize(QSize(48, 48));
    m_listView->setGridSize(QSize(120, 80));
    m_listView->setWordWrap(true);
    m_listView->setSpacing(8);
}

void DeviceDiscoveryWidget::setupToolBar()
{
    auto *toolbar = new QHBoxLayout;

    m_searchBox->setPlaceholderText("Search devices...");
    m_searchBox->setAccessibleName("Device Search");
    m_searchBox->setAccessibleDescription("Filter discovered devices by name, vendor, or protocol");
    m_searchBox->setToolTip("Type to filter the device list");
    m_searchBox->setClearButtonEnabled(true);
    toolbar->addWidget(m_searchBox);

    m_refreshBtn->setText("Refresh");
    m_refreshBtn->setAccessibleName("Refresh Devices");
    m_refreshBtn->setAccessibleDescription("Scan for connected devices");
    m_refreshBtn->setToolTip("Scan for new or changed devices (F5)");
    m_refreshBtn->setShortcut(QKeySequence(Qt::Key_F5));
    toolbar->addWidget(m_refreshBtn);

    m_autoRefreshBtn->setText("Auto");
    m_autoRefreshBtn->setAccessibleName("Auto Refresh");
    m_autoRefreshBtn->setAccessibleDescription("Automatically refresh device list every 5 seconds");
    m_autoRefreshBtn->setToolTip("Toggle automatic device scanning");
    m_autoRefreshBtn->setCheckable(true);
    toolbar->addWidget(m_autoRefreshBtn);

    m_viewModeCombo->addItem("Tree");
    m_viewModeCombo->addItem("List");
    m_viewModeCombo->setAccessibleName("View Mode");
    m_viewModeCombo->setAccessibleDescription("Switch between tree and list device views");
    m_viewModeCombo->setToolTip("Select how to display discovered devices");
    toolbar->addWidget(m_viewModeCombo);

    static_cast<QVBoxLayout*>(layout())->insertLayout(0, toolbar);
}

void DeviceDiscoveryWidget::startDiscovery()
{
    m_discovering = true;
    onRefresh();
    emit discoveryStarted();
}

void DeviceDiscoveryWidget::stopDiscovery()
{
    m_discovering = false;
    m_refreshTimer->stop();
    m_autoRefreshBtn->setChecked(false);
    emit discoveryStopped();
}

void DeviceDiscoveryWidget::setAutoRefresh(bool enabled)
{
    if (enabled) {
        m_refreshTimer->start(5000);
        m_autoRefreshBtn->setChecked(true);
    } else {
        m_refreshTimer->stop();
        m_autoRefreshBtn->setChecked(false);
    }
}

void DeviceDiscoveryWidget::setFilterText(const QString &text)
{
    m_searchBox->setText(text);
}

void DeviceDiscoveryWidget::setViewMode(int mode)
{
    m_viewStack->setCurrentIndex(mode);
    m_viewModeCombo->setCurrentIndex(mode);
}

void DeviceDiscoveryWidget::onRefresh()
{
    if (!m_runtimeBridge || !m_runtimeBridge->isInitialized()) {
        m_statusLabel->setText("Runtime not available");
        return;
    }

    m_refreshBtn->setEnabled(false);
    m_statusLabel->setText("Scanning for devices...");
    QApplication::processEvents();

    emit discoveryStarted();
    m_runtimeBridge->discoverDevices();
}

void DeviceDiscoveryWidget::onSearchChanged(const QString &text)
{
    m_proxyModel->setFilterText(text);
}

void DeviceDiscoveryWidget::onDeviceActivated(const QModelIndex &index)
{
    QString deviceId;
    if (m_viewStack->currentIndex() == 0) {
        auto treeIndex = m_treeView->currentIndex();
        if (!treeIndex.isValid()) return;
        auto node = treeIndex.data(Qt::UserRole).value<std::shared_ptr<DeviceTreeModel::TreeNode>>();
        if (node && node->type == "device") {
            deviceId = node->id;
        }
    } else {
        auto srcIndex = m_proxyModel->mapToSource(index);
        if (srcIndex.isValid()) {
            deviceId = m_listModel->data(srcIndex, DeviceListModel::IdRole).toString();
        }
    }

    if (!deviceId.isEmpty()) {
        auto device = m_listModel->deviceById(deviceId);
        m_detailsWidget->showDevice(device);
        emit deviceSelected(deviceId);
    }
}

void DeviceDiscoveryWidget::onViewModeChanged(int mode)
{
    m_viewStack->setCurrentIndex(mode);
}

void DeviceDiscoveryWidget::onDeviceListChanged(
    const std::vector<gui::runtime::DeviceInfoView>& devices)
{
    QList<DeviceInfo> list;
    for (const auto& dev : devices) {
        DeviceInfo info;
        info.id = QString::fromStdString(dev.connectionPath);
        info.name = QString::fromStdString(dev.friendlyName);
        info.vendor = QString::fromStdString(dev.vendorName);
        info.protocol = QString::fromStdString(dev.protocolName);
        info.transport = QString::fromStdString(dev.transportName);
        info.bootMode = QString::fromStdString(dev.bootModeName);
        info.connected = (dev.connectionStatus == gui::runtime::DeviceConnectionStatus::Connected);
        list.append(info);
    }

    m_listModel->setDevices(list);

    m_discovering = false;
    m_refreshBtn->setEnabled(true);

    if (list.isEmpty()) {
        m_statusLabel->setText("No devices found");
    } else {
        m_statusLabel->setText(QString("%1 device(s) found").arg(list.size()));
    }

    emit discoveryStopped();
}

void DeviceDiscoveryWidget::onDiscoveryFailed(const QString& error)
{
    m_discovering = false;
    m_refreshBtn->setEnabled(true);
    m_statusLabel->setText("Discovery failed: " + error);
    emit discoveryError(error);
    emit discoveryStopped();
}
