#include "gui/framework/ShortcutManager.hpp"
#include "gui/framework/SettingsManager.hpp"
#include <QShortcut>
#include <QWidget>

ShortcutManager::ShortcutManager(QObject *parent) : QObject(parent) {}

void ShortcutManager::registerShortcut(const QString &id, const QKeySequence &defaultKey,
                                        QObject *receiver, const char *slot)
{
    ShortcutEntry entry;
    entry.defaultKey = defaultKey;
    entry.currentKey = defaultKey;
    auto *widget = qobject_cast<QWidget *>(receiver);
    if (widget) {
        entry.shortcut = new QShortcut(defaultKey, widget);
        QObject::connect(entry.shortcut, &QShortcut::activated, receiver, [receiver, slot]() {
            QMetaObject::invokeMethod(receiver, slot);
        });
    }
    m_shortcuts[id] = entry;
}

void ShortcutManager::applyShortcut(const QString &id, const QKeySequence &key)
{
    auto it = m_shortcuts.find(id);
    if (it == m_shortcuts.end()) return;
    it->currentKey = key;
    if (it->shortcut) it->shortcut->setKey(key);
    emit shortcutChanged(id, key);
}

QKeySequence ShortcutManager::shortcut(const QString &id) const
{
    auto it = m_shortcuts.find(id);
    return it != m_shortcuts.end() ? it->currentKey : QKeySequence();
}

void ShortcutManager::restoreDefaults()
{
    for (auto it = m_shortcuts.begin(); it != m_shortcuts.end(); ++it) {
        applyShortcut(it.key(), it->defaultKey);
    }
}

QMap<QString, QKeySequence> ShortcutManager::allShortcuts() const
{
    QMap<QString, QKeySequence> result;
    for (auto it = m_shortcuts.begin(); it != m_shortcuts.end(); ++it)
        result[it.key()] = it->currentKey;
    return result;
}

void ShortcutManager::loadFromSettings()
{
    auto *settings = qobject_cast<SettingsManager *>(parent());
    if (!settings) return;
    settings->beginGroup("shortcuts");
    for (auto &key : settings->allKeys()) {
        auto seq = QKeySequence::fromString(settings->getValue(key).toString());
        applyShortcut(key, seq);
    }
    settings->endGroup();
}

void ShortcutManager::saveToSettings()
{
    auto *settings = qobject_cast<SettingsManager *>(parent());
    if (!settings) return;
    settings->beginGroup("shortcuts");
    for (auto it = m_shortcuts.begin(); it != m_shortcuts.end(); ++it)
        settings->setValue(it.key(), it->currentKey.toString());
    settings->endGroup();
}
