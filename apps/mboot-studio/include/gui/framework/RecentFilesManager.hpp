#pragma once

#include <QObject>
#include <QStringList>
#include <QDateTime>

class RecentFilesManager : public QObject {
    Q_OBJECT
public:
    explicit RecentFilesManager(QObject *parent = nullptr);

    void addFile(const QString &filePath);
    void removeFile(const QString &filePath);
    void clear();
    QStringList recentFiles() const;
    QStringList recentFilesWithTimestamps() const;
    int maxFiles() const { return m_maxFiles; }
    void setMaxFiles(int count);

signals:
    void recentFilesChanged(const QStringList &files);
    void fileAdded(const QString &filePath);
    void fileRemoved(const QString &filePath);
    void cleared();

private:
    QStringList m_files;
    int m_maxFiles{10};
    QMap<QString, QDateTime> m_timestamps;
};
