#pragma once

#include <QObject>
#include <QStack>
#include <QString>
#include <memory>

class CommandManager : public QObject {
    Q_OBJECT
public:
    explicit CommandManager(QObject *parent = nullptr);

    void execute(const QString &command);
    void executeAsync(const QString &command);
    void undo();
    void redo();
    bool canUndo() const;
    bool canRedo() const;
    QString lastCommand() const;
    QStringList commandHistory() const;
    void clearHistory();
    int maxHistory() const { return m_maxHistory; }
    void setMaxHistory(int count);

signals:
    void commandExecuted(const QString &command);
    void commandUndone(const QString &command);
    void commandRedone(const QString &command);
    void historyCleared();
    void canUndoChanged(bool canUndo);
    void canRedoChanged(bool canRedo);

private:
    QStack<QString> m_undoStack;
    QStack<QString> m_redoStack;
    QStringList m_history;
    int m_maxHistory{100};
};
