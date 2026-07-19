#pragma once

#include <QDialog>
#include <QTabWidget>
#include <memory>

class QCheckBox;
class QComboBox;
class QLineEdit;
class QSpinBox;
class QPushButton;

class SettingsDialog : public QDialog {
    Q_OBJECT
public:
    explicit SettingsDialog(QWidget *parent = nullptr);
    void loadSettings();
    void saveSettings();

private slots:
    void onApply();
    void onOk();
    void onCancel();

private:
    void setupUi();
    void setupGeneralTab();
    void setupAppearanceTab();
    void setupRuntimeTab();
    void setupTransportTab();
    void setupPluginsTab();
    void setupLoggingTab();
    void setupUpdatesTab();
    void setupAdvancedTab();

    QTabWidget *m_tabs;
    // General
    QComboBox *m_languageCombo;
    QLineEdit *m_cachePath;
    QSpinBox *m_cacheSize;
    // Appearance
    QComboBox *m_themeCombo;
    QComboBox *m_accentColorCombo;
    // Runtime
    QCheckBox *m_autoDiscover;
    QSpinBox *m_discoveryInterval;
    QCheckBox *m_autoConnect;
    // Transport
    QCheckBox *m_usbEnabled, *m_serialEnabled, *m_tcpEnabled;
    // Plugins
    QCheckBox *m_autoLoadPlugins;
    QLineEdit *m_pluginPath;
    // Logging
    QComboBox *m_logLevel;
    QCheckBox *m_logToFile;
    QLineEdit *m_logPath;
    // Updates
    QCheckBox *m_checkUpdates;
    QSpinBox *m_updateInterval;
    // Advanced
    QCheckBox *m_developerMode;
    QPushButton *m_applyBtn, *m_okBtn, *m_cancelBtn;
};
