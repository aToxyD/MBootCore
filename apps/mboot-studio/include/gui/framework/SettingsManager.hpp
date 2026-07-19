#pragma once

#include <QObject>
#include <QSettings>
#include <QString>
#include <QVariant>
#include <memory>

class SettingsManager : public QObject {
    Q_OBJECT
public:
    explicit SettingsManager(QObject *parent = nullptr);

    QVariant getValue(const QString &key, const QVariant &defaultValue = {}) const;
    void setValue(const QString &key, const QVariant &value);
    void removeValue(const QString &key);
    bool contains(const QString &key) const;
    void sync();

    QStringList allKeys() const;
    void beginGroup(const QString &prefix);
    void endGroup();
    QString group() const;

    // App settings
    QString theme() const;
    void setTheme(const QString &theme);
    QColor accentColor() const;
    void setAccentColor(const QColor &color);
    QByteArray windowGeometry() const;
    void setWindowGeometry(const QByteArray &geometry);
    QByteArray windowState() const;
    void setWindowState(const QByteArray &state);
    QStringList recentFiles() const;
    void setRecentFiles(const QStringList &files);
    QString language() const;
    void setLanguage(const QString &locale);
    bool developerMode() const;
    void setDeveloperMode(bool enabled);
    bool checkUpdates() const;
    void setCheckUpdates(bool enabled);

signals:
    void settingChanged(const QString &key, const QVariant &value);

private:
    std::unique_ptr<QSettings> m_settings;
};
