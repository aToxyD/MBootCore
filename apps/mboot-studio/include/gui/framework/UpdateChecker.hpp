#pragma once

#include <QObject>
#include <QString>
#include <QVersionNumber>
#include <memory>

class QNetworkAccessManager;
class QNetworkReply;

class UpdateChecker : public QObject {
    Q_OBJECT
public:
    struct UpdateInfo {
        QVersionNumber version;
        QString downloadUrl;
        QString releaseNotes;
        QString releaseDate;
        bool isCritical{false};
    };

    explicit UpdateChecker(QObject *parent = nullptr);

    void checkForUpdates();
    void checkForUpdatesAsync();
    bool isUpdateAvailable() const { return m_updateAvailable; }
    UpdateInfo latestVersion() const { return m_latestVersion; }
    QString currentVersion() const;
    void setUpdateUrl(const QString &url);

signals:
    void updateAvailable(const UpdateInfo &info);
    void upToDate();
    void checkFailed(const QString &error);
    void checkStarted();
    void checkFinished();

private:
    void parseResponse(const QByteArray &data);

    QNetworkAccessManager *m_networkManager;
    UpdateInfo m_latestVersion;
    bool m_updateAvailable{false};
    QString m_updateUrl;
};
