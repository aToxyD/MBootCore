#pragma once

#include <QObject>
#include <QMap>
#include <QString>
#include <QKeySequence>
#include <QAction>
#include <memory>

class QAction;
class QMenu;
class QMenuBar;
class QToolBar;

class ActionManager : public QObject {
    Q_OBJECT
public:
    explicit ActionManager(QObject *parent = nullptr);

    QAction *registerAction(const QString &id, QAction *action);
    QAction *createAction(const QString &id, const QString &text, const QKeySequence &shortcut = QKeySequence());
    QAction *findAction(const QString &id) const;
    void removeAction(const QString &id);

    QMenu *createMenu(const QString &title, QMenuBar *menuBar = nullptr);
    QMenu *findMenu(const QString &title) const;
    QToolBar *createToolBar(const QString &title);

    void setActionEnabled(const QString &id, bool enabled);
    void setActionVisible(const QString &id, bool visible);
    void setActionText(const QString &id, const QString &text);

    QStringList actionIds() const { return m_actions.keys(); }

signals:
    void actionTriggered(const QString &id);

private:
    QMap<QString, QAction *> m_actions;
    QList<QMenu *> m_menus;
    QList<QToolBar *> m_toolBars;
};
