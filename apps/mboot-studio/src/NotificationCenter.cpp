#include "gui/framework/NotificationCenter.hpp"
#include <QUuid>

NotificationCenter::NotificationCenter(QObject *parent) : QObject(parent) {}

QString NotificationCenter::post(const QString &title, const QString &message, Severity severity)
{
    Notification n;
    n.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    n.title = title;
    n.message = message;
    n.severity = severity;
    n.timestamp = QDateTime::currentDateTime();
    m_notifications.append(n);
    ++m_unreadCount;
    emit notificationPosted(n);
    emit unreadCountChanged(m_unreadCount);
    return n.id;
}

void NotificationCenter::dismiss(const QString &id)
{
    for (auto &n : m_notifications) {
        if (n.id == id) { n.dismissed = true; break; }
    }
    emit notificationDismissed(id);
}

void NotificationCenter::dismissAll()
{
    for (auto &n : m_notifications) n.dismissed = true;
    emit allDismissed();
}

void NotificationCenter::clear()
{
    m_notifications.clear();
    m_unreadCount = 0;
    emit cleared();
    emit unreadCountChanged(0);
}

QList<NotificationCenter::Notification> NotificationCenter::notifications() const { return m_notifications; }

QList<NotificationCenter::Notification> NotificationCenter::activeNotifications() const
{
    QList<Notification> active;
    for (const auto &n : m_notifications) if (!n.dismissed) active.append(n);
    return active;
}

int NotificationCenter::count() const { return static_cast<int>(m_notifications.size()); }
int NotificationCenter::unreadCount() const { return m_unreadCount; }

void NotificationCenter::markAllRead()
{
    m_unreadCount = 0;
    emit unreadCountChanged(0);
}
