#include "gui/framework/ActionManager.hpp"
#include <QAction>
#include <QMenu>
#include <QMenuBar>
#include <QToolBar>

ActionManager::ActionManager(QObject *parent) : QObject(parent) {}

QAction *ActionManager::registerAction(const QString &id, QAction *action)
{
    m_actions[id] = action;
    connect(action, &QAction::triggered, this, [this, id]() {
        emit actionTriggered(id);
    });
    return action;
}

QAction *ActionManager::createAction(const QString &id, const QString &text, const QKeySequence &shortcut)
{
    auto *action = new QAction(text, this);
    action->setShortcut(shortcut);
    return registerAction(id, action);
}

QAction *ActionManager::findAction(const QString &id) const
{
    return m_actions.value(id, nullptr);
}

void ActionManager::removeAction(const QString &id)
{
    auto it = m_actions.find(id);
    if (it != m_actions.end()) {
        delete it.value();
        m_actions.erase(it);
    }
}

QMenu *ActionManager::createMenu(const QString &title, QMenuBar *menuBar)
{
    auto *menu = new QMenu(title);
    m_menus.append(menu);
    if (menuBar) menuBar->addMenu(menu);
    return menu;
}

QMenu *ActionManager::findMenu(const QString &title) const
{
    for (auto *menu : m_menus) {
        if (menu->title() == title) return menu;
    }
    return nullptr;
}

QToolBar *ActionManager::createToolBar(const QString &title)
{
    auto *toolBar = new QToolBar(title);
    m_toolBars.append(toolBar);
    return toolBar;
}

void ActionManager::setActionEnabled(const QString &id, bool enabled)
{
    if (auto *action = findAction(id)) action->setEnabled(enabled);
}

void ActionManager::setActionVisible(const QString &id, bool visible)
{
    if (auto *action = findAction(id)) action->setVisible(visible);
}

void ActionManager::setActionText(const QString &id, const QString &text)
{
    if (auto *action = findAction(id)) action->setText(text);
}
