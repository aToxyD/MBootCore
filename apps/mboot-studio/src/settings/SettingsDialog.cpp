#include "gui/settings/SettingsDialog.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QCheckBox>
#include <QComboBox>
#include <QLineEdit>
#include <QSpinBox>
#include <QPushButton>
#include <QLabel>
#include <QDialogButtonBox>

SettingsDialog::SettingsDialog(QWidget *parent)
    : QDialog(parent)
    , m_tabs(new QTabWidget(this))
    , m_languageCombo(new QComboBox(this))
    , m_cachePath(new QLineEdit(this))
    , m_cacheSize(new QSpinBox(this))
    , m_themeCombo(new QComboBox(this))
    , m_accentColorCombo(new QComboBox(this))
    , m_autoDiscover(new QCheckBox(tr("Auto Discover Devices"), this))
    , m_discoveryInterval(new QSpinBox(this))
    , m_autoConnect(new QCheckBox(tr("Auto Connect"), this))
    , m_usbEnabled(new QCheckBox(tr("USB"), this))
    , m_serialEnabled(new QCheckBox(tr("Serial"), this))
    , m_tcpEnabled(new QCheckBox(tr("TCP"), this))
    , m_autoLoadPlugins(new QCheckBox(tr("Auto Load Plugins"), this))
    , m_pluginPath(new QLineEdit(this))
    , m_logLevel(new QComboBox(this))
    , m_logToFile(new QCheckBox(tr("Log to File"), this))
    , m_logPath(new QLineEdit(this))
    , m_checkUpdates(new QCheckBox(tr("Check for Updates"), this))
    , m_updateInterval(new QSpinBox(this))
    , m_developerMode(new QCheckBox(tr("Developer Mode"), this))
    , m_applyBtn(new QPushButton(tr("Apply"), this))
    , m_okBtn(new QPushButton(tr("OK"), this))
    , m_cancelBtn(new QPushButton(tr("Cancel"), this))
{
    setupUi();
}

void SettingsDialog::setupUi()
{
    setAccessibleName("Settings");
    setAccessibleDescription("Application settings and preferences");

    auto *layout = new QVBoxLayout(this);
    setupGeneralTab();
    setupAppearanceTab();
    setupRuntimeTab();
    setupTransportTab();
    setupPluginsTab();
    setupLoggingTab();
    setupUpdatesTab();
    setupAdvancedTab();
    layout->addWidget(m_tabs);

    m_applyBtn->setAccessibleName("Apply Settings");
    m_applyBtn->setAccessibleDescription("Apply settings changes without closing the dialog");
    m_applyBtn->setToolTip("Apply changes");

    m_okBtn->setAccessibleName("OK");
    m_okBtn->setAccessibleDescription("Save settings and close the dialog");
    m_okBtn->setDefault(true);

    m_cancelBtn->setAccessibleName("Cancel");
    m_cancelBtn->setAccessibleDescription("Discard changes and close the dialog");

    auto *btnLayout = new QHBoxLayout();
    btnLayout->addStretch();
    btnLayout->addWidget(m_okBtn);
    btnLayout->addWidget(m_applyBtn);
    btnLayout->addWidget(m_cancelBtn);
    layout->addLayout(btnLayout);

    QWidget::setTabOrder(m_tabs, m_okBtn);
    QWidget::setTabOrder(m_okBtn, m_applyBtn);
    QWidget::setTabOrder(m_applyBtn, m_cancelBtn);

    connect(m_applyBtn, &QPushButton::clicked, this, &SettingsDialog::onApply);
    connect(m_okBtn, &QPushButton::clicked, this, &SettingsDialog::onOk);
    connect(m_cancelBtn, &QPushButton::clicked, this, &SettingsDialog::onCancel);
}

void SettingsDialog::setupGeneralTab()
{
    auto *w = new QWidget(this);
    auto *f = new QFormLayout(w);
    m_languageCombo->setAccessibleName("Language");
    m_languageCombo->setAccessibleDescription("Select application language");
    m_languageCombo->addItems({tr("English"), tr("中文"), tr("日本語")});
    f->addRow(tr("Language"), m_languageCombo);
    m_cachePath->setAccessibleName("Cache Path");
    m_cachePath->setAccessibleDescription("Path for application cache files");
    m_cachePath->setPlaceholderText(tr("Default cache directory"));
    f->addRow(tr("Cache Path"), m_cachePath);
    m_cacheSize->setAccessibleName("Cache Size");
    m_cacheSize->setAccessibleDescription("Maximum cache size in megabytes");
    m_cacheSize->setRange(0, 10000);
    m_cacheSize->setSuffix(" MB");
    f->addRow(tr("Cache Size"), m_cacheSize);
    m_tabs->addTab(w, tr("General"));
}

void SettingsDialog::setupAppearanceTab()
{
    auto *w = new QWidget(this);
    auto *f = new QFormLayout(w);
    m_themeCombo->setAccessibleName("Theme");
    m_themeCombo->setAccessibleDescription("Select application color theme");
    m_themeCombo->addItems({tr("Light"), tr("Dark"), tr("System")});
    f->addRow(tr("Theme"), m_themeCombo);
    m_accentColorCombo->setAccessibleName("Accent Color");
    m_accentColorCombo->setAccessibleDescription("Select accent color for the UI");
    m_accentColorCombo->addItems({tr("Blue"), tr("Green"), tr("Purple"), tr("Orange")});
    f->addRow(tr("Accent Color"), m_accentColorCombo);
    m_tabs->addTab(w, tr("Appearance"));
}

void SettingsDialog::setupRuntimeTab()
{
    auto *w = new QWidget(this);
    auto *f = new QFormLayout(w);
    m_autoDiscover->setAccessibleName("Auto Discover");
    m_autoDiscover->setAccessibleDescription("Automatically scan for devices at startup");
    f->addRow(m_autoDiscover);
    m_discoveryInterval->setAccessibleName("Discovery Interval");
    m_discoveryInterval->setAccessibleDescription("Time between automatic device scans in seconds");
    m_discoveryInterval->setRange(1, 60);
    m_discoveryInterval->setValue(5);
    m_discoveryInterval->setSuffix(" s");
    f->addRow(tr("Discovery Interval"), m_discoveryInterval);
    m_autoConnect->setAccessibleName("Auto Connect");
    m_autoConnect->setAccessibleDescription("Automatically connect to the first discovered device");
    f->addRow(m_autoConnect);
    m_tabs->addTab(w, tr("Runtime"));
}

void SettingsDialog::setupTransportTab()
{
    auto *w = new QWidget(this);
    auto *f = new QFormLayout(w);
    m_usbEnabled->setAccessibleName("Enable USB");
    m_usbEnabled->setAccessibleDescription("Enable USB transport for device communication");
    f->addRow(m_usbEnabled);
    m_serialEnabled->setAccessibleName("Enable Serial");
    m_serialEnabled->setAccessibleDescription("Enable Serial/COM port transport");
    f->addRow(m_serialEnabled);
    m_tcpEnabled->setAccessibleName("Enable TCP");
    m_tcpEnabled->setAccessibleDescription("Enable TCP/IP network transport");
    f->addRow(m_tcpEnabled);
    m_tabs->addTab(w, tr("Transport"));
}

void SettingsDialog::setupPluginsTab()
{
    auto *w = new QWidget(this);
    auto *f = new QFormLayout(w);
    m_autoLoadPlugins->setAccessibleName("Auto Load Plugins");
    m_autoLoadPlugins->setAccessibleDescription("Automatically load plugins at startup");
    f->addRow(m_autoLoadPlugins);
    m_pluginPath->setAccessibleName("Plugin Path");
    m_pluginPath->setAccessibleDescription("Directory to search for plugin files");
    m_pluginPath->setPlaceholderText(tr("Plugin search directory"));
    f->addRow(tr("Plugin Path"), m_pluginPath);
    m_tabs->addTab(w, tr("Plugins"));
}

void SettingsDialog::setupLoggingTab()
{
    auto *w = new QWidget(this);
    auto *f = new QFormLayout(w);
    m_logLevel->setAccessibleName("Log Level");
    m_logLevel->setAccessibleDescription("Minimum log level to display");
    m_logLevel->addItems({tr("Debug"), tr("Info"), tr("Warning"), tr("Error")});
    f->addRow(tr("Log Level"), m_logLevel);
    m_logToFile->setAccessibleName("Log to File");
    m_logToFile->setAccessibleDescription("Write log messages to a file");
    f->addRow(m_logToFile);
    m_logPath->setAccessibleName("Log Path");
    m_logPath->setAccessibleDescription("File path for log output");
    m_logPath->setPlaceholderText(tr("Log output file path"));
    f->addRow(tr("Log Path"), m_logPath);
    m_tabs->addTab(w, tr("Logging"));
}

void SettingsDialog::setupUpdatesTab()
{
    auto *w = new QWidget(this);
    auto *f = new QFormLayout(w);
    m_checkUpdates->setAccessibleName("Check for Updates");
    m_checkUpdates->setAccessibleDescription("Automatically check for application updates");
    f->addRow(m_checkUpdates);
    m_updateInterval->setAccessibleName("Update Interval");
    m_updateInterval->setAccessibleDescription("Hours between automatic update checks");
    m_updateInterval->setRange(1, 168);
    m_updateInterval->setValue(24);
    m_updateInterval->setSuffix(" h");
    f->addRow(tr("Update Interval"), m_updateInterval);
    m_tabs->addTab(w, tr("Updates"));
}

void SettingsDialog::setupAdvancedTab()
{
    auto *w = new QWidget(this);
    auto *f = new QFormLayout(w);
    m_developerMode->setAccessibleName("Developer Mode");
    m_developerMode->setAccessibleDescription("Enable developer mode with additional debugging tools");
    f->addRow(m_developerMode);
    m_tabs->addTab(w, tr("Advanced"));
}

void SettingsDialog::loadSettings()
{
}

void SettingsDialog::saveSettings()
{
}

void SettingsDialog::onApply()
{
    saveSettings();
}

void SettingsDialog::onOk()
{
    saveSettings();
    accept();
}

void SettingsDialog::onCancel()
{
    reject();
}
