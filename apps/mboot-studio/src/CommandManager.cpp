#include "gui/framework/CommandManager.hpp"

CommandManager::CommandManager(QObject *parent) : QObject(parent) {}

void CommandManager::execute(const QString &command)
{
    m_undoStack.push(command);
    m_redoStack.clear();
    m_history.append(command);
    if (m_history.size() > m_maxHistory) m_history.removeFirst();
    emit commandExecuted(command);
    emit canUndoChanged(true);
    emit canRedoChanged(false);
}

void CommandManager::executeAsync(const QString &command)
{
    execute(command);
}

void CommandManager::undo()
{
    if (m_undoStack.isEmpty()) return;
    auto cmd = m_undoStack.pop();
    m_redoStack.push(cmd);
    emit commandUndone(cmd);
    emit canUndoChanged(!m_undoStack.isEmpty());
    emit canRedoChanged(true);
}

void CommandManager::redo()
{
    if (m_redoStack.isEmpty()) return;
    auto cmd = m_redoStack.pop();
    m_undoStack.push(cmd);
    emit commandRedone(cmd);
    emit canUndoChanged(true);
    emit canRedoChanged(!m_redoStack.isEmpty());
}

bool CommandManager::canUndo() const { return !m_undoStack.isEmpty(); }
bool CommandManager::canRedo() const { return !m_redoStack.isEmpty(); }
QString CommandManager::lastCommand() const { return m_undoStack.isEmpty() ? "" : m_undoStack.top(); }
QStringList CommandManager::commandHistory() const { return m_history; }

void CommandManager::clearHistory()
{
    m_undoStack.clear();
    m_redoStack.clear();
    m_history.clear();
    emit historyCleared();
    emit canUndoChanged(false);
    emit canRedoChanged(false);
}

void CommandManager::setMaxHistory(int count) { m_maxHistory = qMax(1, count); }
