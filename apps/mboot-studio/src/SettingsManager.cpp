#include "gui/framework/SettingsManager.hpp"
#include <QStandardPaths>
#include <QDir>
#include <QColor>

SettingsManager::SettingsManager(QObject *parent)
    : QObject(parent)
{
    QString configPath = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QDir().mkpath(configPath);
    m_settings = std::make_unique<QSettings>(configPath + "/mboot-studio.ini", QSettings::IniFormat);
}

QVariant SettingsManager::getValue(const QString &key, const QVariant &defaultValue) const
{
    return m_settings->value(key, defaultValue);
}

void SettingsManager::setValue(const QString &key, const QVariant &value)
{
    m_settings->setValue(key, value);
    emit settingChanged(key, value);
}

void SettingsManager::removeValue(const QString &key)
{
    m_settings->remove(key);
}

bool SettingsManager::contains(const QString &key) const
{
    return m_settings->contains(key);
}

void SettingsManager::sync()
{
    m_settings->sync();
}

QStringList SettingsManager::allKeys() const
{
    return m_settings->allKeys();
}

void SettingsManager::beginGroup(const QString &prefix)
{
    m_settings->beginGroup(prefix);
}

void SettingsManager::endGroup()
{
    m_settings->endGroup();
}

QString SettingsManager::group() const
{
    return m_settings->group();
}

QString SettingsManager::theme() const { return getValue("appearance/theme", "Dark").toString(); }
void SettingsManager::setTheme(const QString &theme) { setValue("appearance/theme", theme); }
QColor SettingsManager::accentColor() const { return QColor(getValue("appearance/accentColor", "#00aaff").toString()); }
void SettingsManager::setAccentColor(const QColor &color) { setValue("appearance/accentColor", color.name()); }
QByteArray SettingsManager::windowGeometry() const { return getValue("window/geometry").toByteArray(); }
void SettingsManager::setWindowGeometry(const QByteArray &g) { setValue("window/geometry", g); }
QByteArray SettingsManager::windowState() const { return getValue("window/state").toByteArray(); }
void SettingsManager::setWindowState(const QByteArray &s) { setValue("window/state", s); }
QStringList SettingsManager::recentFiles() const { return getValue("files/recent", QStringList()).toStringList(); }
void SettingsManager::setRecentFiles(const QStringList &f) { setValue("files/recent", f); }
QString SettingsManager::language() const { return getValue("locale/language", "en").toString(); }
void SettingsManager::setLanguage(const QString &l) { setValue("locale/language", l); }
bool SettingsManager::developerMode() const { return getValue("advanced/developerMode", false).toBool(); }
void SettingsManager::setDeveloperMode(bool e) { setValue("advanced/developerMode", e); }
bool SettingsManager::checkUpdates() const { return getValue("updates/checkEnabled", true).toBool(); }
void SettingsManager::setCheckUpdates(bool e) { setValue("updates/checkEnabled", e); }
