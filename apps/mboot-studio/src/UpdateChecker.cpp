#include "gui/framework/UpdateChecker.hpp"
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QApplication>

UpdateChecker::UpdateChecker(QObject *parent)
    : QObject(parent)
{
    m_networkManager = new QNetworkAccessManager(this);
    m_updateUrl = "https://api.github.com/repos/mbootcore/mboot-studio/releases/latest";
}

void UpdateChecker::checkForUpdates()
{
    emit checkStarted();
    QUrl url(m_updateUrl);
    QNetworkRequest request{url};
    request.setHeader(QNetworkRequest::UserAgentHeader, "MBootStudio");
    QNetworkReply *reply = m_networkManager->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        if (reply->error() == QNetworkReply::NoError) {
            parseResponse(reply->readAll());
        } else {
            emit checkFailed(reply->errorString());
        }
        reply->deleteLater();
        emit checkFinished();
    });
}

void UpdateChecker::checkForUpdatesAsync() { checkForUpdates(); }

void UpdateChecker::parseResponse(const QByteArray &data)
{
    auto doc = QJsonDocument::fromJson(data);
    if (!doc.isObject()) { emit checkFailed("Invalid response"); return; }
    auto obj = doc.object();
    auto tag = obj["tag_name"].toString().remove('v');
    m_latestVersion.version = QVersionNumber::fromString(tag);
    m_latestVersion.downloadUrl = obj["html_url"].toString();
    m_latestVersion.releaseNotes = obj["body"].toString();
    m_latestVersion.releaseDate = obj["published_at"].toString();

    auto current = QVersionNumber::fromString(QApplication::applicationVersion());
    if (!m_latestVersion.version.isNull() && m_latestVersion.version > current) {
        m_updateAvailable = true;
        emit updateAvailable(m_latestVersion);
    } else {
        m_updateAvailable = false;
        emit upToDate();
    }
}

QString UpdateChecker::currentVersion() const
{
    return QApplication::applicationVersion();
}

void UpdateChecker::setUpdateUrl(const QString &url) { m_updateUrl = url; }
