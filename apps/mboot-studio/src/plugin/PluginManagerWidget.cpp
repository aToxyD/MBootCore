#include "gui/plugin/PluginManagerWidget.hpp"
#include "gui/runtime/RuntimeBridge.hpp"
#include "gui/runtime/RuntimeEventDispatcher.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QFileDialog>
#include <QTabWidget>

PluginManagerWidget::PluginManagerWidget(QWidget *parent)
    : QWidget(parent)
{
    setupUi();
}

void PluginManagerWidget::setRuntimeBridge(gui::runtime::RuntimeBridge *bridge)
{
    if (m_bridge) return;
    m_bridge = bridge;

    auto *dispatcher = m_bridge->eventDispatcher();
    connect(dispatcher, &gui::runtime::RuntimeEventDispatcher::pluginListChanged,
            this, &PluginManagerWidget::onPluginListChanged);
    connect(m_bridge, &gui::runtime::RuntimeBridge::pluginLoaded,
            this, &PluginManagerWidget::onPluginLoaded);
    connect(m_bridge, &gui::runtime::RuntimeBridge::pluginUnloaded,
            this, &PluginManagerWidget::onPluginUnloaded);
    connect(dispatcher, &gui::runtime::RuntimeEventDispatcher::capabilitiesChanged,
            this, &PluginManagerWidget::onCapabilitiesChanged);
}

void PluginManagerWidget::loadPlugins()
{
    if (m_bridge) {
        m_plugins = m_bridge->enumeratePlugins();
        m_capabilities = m_bridge->pluginCapabilities();
        updatePluginTable();
        m_statusLabel->setText(tr("Loaded %1 plugin(s), %2 capabilities")
            .arg(m_plugins.size()).arg(m_capabilities.size()));
    }
}

void PluginManagerWidget::unloadPlugin(const QString &id)
{
    if (m_bridge && !id.isEmpty()) {
        m_bridge->unloadPlugin(id.toStdString());
    }
}

void PluginManagerWidget::enablePlugin(const QString &id)
{
    Q_UNUSED(id)
    m_statusLabel->setText(tr("Enable requires plugin context (not available through public API)"));
}

void PluginManagerWidget::disablePlugin(const QString &id)
{
    Q_UNUSED(id)
    m_statusLabel->setText(tr("Disable requires plugin context (not available through public API)"));
}

void PluginManagerWidget::onPluginListChanged()
{
    if (m_bridge) {
        m_plugins = m_bridge->enumeratePlugins();
        updatePluginTable();
        m_statusLabel->setText(tr("Plugin list updated: %1 plugin(s)").arg(m_plugins.size()));
    }
}

void PluginManagerWidget::onPluginLoaded(const QString &name)
{
    Q_UNUSED(name)
    onPluginListChanged();
}

void PluginManagerWidget::onPluginUnloaded(const QString &name)
{
    Q_UNUSED(name)
    onPluginListChanged();
}

void PluginManagerWidget::onCapabilitiesChanged()
{
    if (m_bridge) {
        m_capabilities = m_bridge->pluginCapabilities();
        m_capabilitiesTable->setRowCount(0);
        for (const auto& cap : m_capabilities) {
            int row = m_capabilitiesTable->rowCount();
            m_capabilitiesTable->insertRow(row);
            m_capabilitiesTable->setItem(row, 0,
                new QTableWidgetItem(QString::fromStdString(cap.name)));
            m_capabilitiesTable->setItem(row, 1,
                new QTableWidgetItem(QString::fromStdString(cap.description)));
            auto *statusItem = new QTableWidgetItem(cap.available ? tr("Available") : tr("Unavailable"));
            m_capabilitiesTable->setItem(row, 2, statusItem);
        }
    }
}

void PluginManagerWidget::updatePluginTable()
{
    m_table->setRowCount(0);
    for (const auto& plugin : m_plugins) {
        int row = m_table->rowCount();
        m_table->insertRow(row);
        m_table->setItem(row, 0, new QTableWidgetItem(QString::fromStdString(plugin.name)));
        m_table->setItem(row, 1, new QTableWidgetItem(QString::fromStdString(plugin.version)));
        m_table->setItem(row, 2, new QTableWidgetItem(QString::fromStdString(plugin.vendor)));
        m_table->setItem(row, 3, new QTableWidgetItem(
            plugin.enabled ? tr("Yes") : tr("No")));
    }
    m_capabilitiesTable->setRowCount(0);
    for (const auto& cap : m_capabilities) {
        int row = m_capabilitiesTable->rowCount();
        m_capabilitiesTable->insertRow(row);
        m_capabilitiesTable->setItem(row, 0,
            new QTableWidgetItem(QString::fromStdString(cap.name)));
        m_capabilitiesTable->setItem(row, 1,
            new QTableWidgetItem(QString::fromStdString(cap.description)));
        auto *statusItem = new QTableWidgetItem(cap.available ? tr("Available") : tr("Unavailable"));
        m_capabilitiesTable->setItem(row, 2, statusItem);
    }
}

void PluginManagerWidget::setupUi()
{
    auto *layout = new QVBoxLayout(this);

    auto *btnLayout = new QHBoxLayout();
    m_loadBtn = new QPushButton(tr("Refresh"), this);
    m_loadBtn->setAccessibleName("Refresh Plugins");
    m_loadBtn->setAccessibleDescription("Reload plugin list from the runtime");
    m_loadBtn->setToolTip("Refresh the plugin and capability list");
    m_loadBtn->setDefault(true);
    btnLayout->addWidget(m_loadBtn);

    m_unloadBtn = new QPushButton(tr("Unload"), this);
    m_unloadBtn->setAccessibleName("Unload Plugin");
    m_unloadBtn->setAccessibleDescription("Unload the selected plugin from the runtime");
    m_unloadBtn->setToolTip("Unload the selected plugin");
    btnLayout->addWidget(m_unloadBtn);

    m_enableBtn = new QPushButton(tr("Enable"), this);
    m_enableBtn->setAccessibleName("Enable Plugin");
    m_enableBtn->setAccessibleDescription("Enable the selected plugin (requires direct plugin manager access)");
    m_enableBtn->setToolTip("Enable the selected plugin (not available through public API)");
    btnLayout->addWidget(m_enableBtn);

    m_disableBtn = new QPushButton(tr("Disable"), this);
    m_disableBtn->setAccessibleName("Disable Plugin");
    m_disableBtn->setAccessibleDescription("Disable the selected plugin (requires direct plugin manager access)");
    m_disableBtn->setToolTip("Disable the selected plugin (not available through public API)");
    btnLayout->addWidget(m_disableBtn);
    layout->addLayout(btnLayout);

    auto *tabs = new QTabWidget(this);

    m_table = new QTableWidget(this);
    m_table->setColumnCount(4);
    m_table->setHorizontalHeaderLabels({tr("Name"), tr("Version"), tr("Vendor"), tr("Enabled")});
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setAccessibleName("Plugin List");
    m_table->setAccessibleDescription("List of installed plugins with name, version, vendor, and enabled status");
    tabs->addTab(m_table, tr("Plugins"));

    m_capabilitiesTable = new QTableWidget(this);
    m_capabilitiesTable->setColumnCount(3);
    m_capabilitiesTable->setHorizontalHeaderLabels({tr("Capability"), tr("Description"), tr("Status")});
    m_capabilitiesTable->horizontalHeader()->setStretchLastSection(true);
    m_capabilitiesTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_capabilitiesTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_capabilitiesTable->setAccessibleName("Capability List");
    m_capabilitiesTable->setAccessibleDescription("Available runtime capabilities and their status");
    tabs->addTab(m_capabilitiesTable, tr("Capabilities"));

    layout->addWidget(tabs);

    m_statusLabel = new QLabel(tr("Ready"), this);
    m_statusLabel->setAccessibleName("Plugin Status");
    m_statusLabel->setAccessibleDescription("Current plugin management status message");
    layout->addWidget(m_statusLabel);

    QWidget::setTabOrder(m_loadBtn, m_unloadBtn);
    QWidget::setTabOrder(m_unloadBtn, m_enableBtn);
    QWidget::setTabOrder(m_enableBtn, m_disableBtn);

    connect(m_loadBtn, &QPushButton::clicked, this, &PluginManagerWidget::loadPlugins);
    connect(m_unloadBtn, &QPushButton::clicked, this, [this]() {
        auto selected = m_table->selectedItems();
        if (!selected.isEmpty())
            unloadPlugin(m_table->item(selected.first()->row(), 0)->text());
    });
    connect(m_enableBtn, &QPushButton::clicked, this, [this]() {
        auto selected = m_table->selectedItems();
        if (!selected.isEmpty())
            enablePlugin(m_table->item(selected.first()->row(), 0)->text());
    });
    connect(m_disableBtn, &QPushButton::clicked, this, [this]() {
        auto selected = m_table->selectedItems();
        if (!selected.isEmpty())
            disablePlugin(m_table->item(selected.first()->row(), 0)->text());
    });
}
