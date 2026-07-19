#pragma once

#include <QObject>
#include <QString>
#include <QDateTime>
#include <QList>
#include <memory>

class NotificationCenter : public QObject {
    Q_OBJECT
public:
    enum Severity { Info, Warning, Error, Success };
    Q_ENUM(Severity)

    struct Notification {
        QString id;
        QString title;
        QString message;
        Severity severity{Info};
        QDateTime timestamp;
        bool dismissed{false};
    };

    explicit NotificationCenter(QObject *parent = nullptr);

    QString post(const QString &title, const QString &message, Severity severity = Info);
    void dismiss(const QString &id);
    void dismissAll();
    void clear();
    QList<Notification> notifications() const;
    QList<Notification> activeNotifications() const;
    int count() const;
    int unreadCount() const;
    void markAllRead();

signals:
    void notificationPosted(const Notification &notification);
    void notificationDismissed(const QString &id);
    void allDismissed();
    void cleared();
    void unreadCountChanged(int count);

private:
    QList<Notification> m_notifications;
    int m_unreadCount{0};
    int m_nextId{1};
};
