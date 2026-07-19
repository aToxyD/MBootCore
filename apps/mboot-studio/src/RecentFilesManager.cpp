#include "gui/framework/RecentFilesManager.hpp"
#include "gui/framework/SettingsManager.hpp"
#include <QFileInfo>
#include <algorithm>

RecentFilesManager::RecentFilesManager(QObject *parent) : QObject(parent)
{
    auto *settings = qobject_cast<SettingsManager *>(parent);
    if (settings) m_files = settings->recentFiles();
}

void RecentFilesManager::addFile(const QString &filePath)
{
    m_files.removeAll(filePath);
    m_files.prepend(filePath);
    m_timestamps[filePath] = QDateTime::currentDateTime();
    if (m_files.size() > m_maxFiles) {
        auto removed = m_files.takeLast();
        m_timestamps.remove(removed);
    }
    auto *settings = qobject_cast<SettingsManager *>(parent());
    if (settings) settings->setRecentFiles(m_files);
    emit recentFilesChanged(m_files);
    emit fileAdded(filePath);
}

void RecentFilesManager::removeFile(const QString &filePath)
{
    m_files.removeAll(filePath);
    m_timestamps.remove(filePath);
    auto *settings = qobject_cast<SettingsManager *>(parent());
    if (settings) settings->setRecentFiles(m_files);
    emit recentFilesChanged(m_files);
    emit fileRemoved(filePath);
}

void RecentFilesManager::clear()
{
    m_files.clear();
    m_timestamps.clear();
    auto *settings = qobject_cast<SettingsManager *>(parent());
    if (settings) settings->setRecentFiles(m_files);
    emit recentFilesChanged(m_files);
    emit cleared();
}

QStringList RecentFilesManager::recentFiles() const { return m_files; }

QStringList RecentFilesManager::recentFilesWithTimestamps() const
{
    QStringList result;
    for (const auto &f : m_files) {
        auto it = m_timestamps.find(f);
        if (it != m_timestamps.end())
            result.append(QString("%1 [%2]").arg(f, it.value().toString(Qt::ISODate)));
        else
            result.append(f);
    }
    return result;
}

void RecentFilesManager::setMaxFiles(int count)
{
    m_maxFiles = qBound(1, count, 50);
    while (m_files.size() > m_maxFiles) {
        auto removed = m_files.takeLast();
        m_timestamps.remove(removed);
    }
}
