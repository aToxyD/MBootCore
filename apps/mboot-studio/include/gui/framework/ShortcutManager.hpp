#pragma once

#include <QObject>
#include <QMap>
#include <QKeySequence>
#include <QString>

class QShortcut;

class ShortcutManager : public QObject {
    Q_OBJECT
public:
    explicit ShortcutManager(QObject *parent = nullptr);

    void registerShortcut(const QString &id, const QKeySequence &defaultKey, QObject *receiver, const char *slot);
    void applyShortcut(const QString &id, const QKeySequence &key);
    QKeySequence shortcut(const QString &id) const;
    void restoreDefaults();
    QMap<QString, QKeySequence> allShortcuts() const;
    void loadFromSettings();
    void saveToSettings();

signals:
    void shortcutChanged(const QString &id, const QKeySequence &key);

private:
    struct ShortcutEntry {
        QKeySequence defaultKey;
        QKeySequence currentKey;
        QShortcut *shortcut{nullptr};
    };
    QMap<QString, ShortcutEntry> m_shortcuts;
};
