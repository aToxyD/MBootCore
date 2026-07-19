#pragma once

#include <QObject>
#include <QByteArray>
#include <QMap>
#include <QString>

class QMainWindow;
class QDockWidget;

class WindowStateManager : public QObject {
    Q_OBJECT
public:
    explicit WindowStateManager(QObject *parent = nullptr);

    void registerMainWindow(QMainWindow *window);
    void registerDockWidget(const QString &name, QDockWidget *dock);
    void saveState();
    void restoreState();
    void saveLayout(const QString &preset);
    void restoreLayout(const QString &preset);
    QStringList availablePresets() const;
    void resetLayout();
    void setAutoSave(bool enabled) { m_autoSave = enabled; }
    bool autoSave() const { return m_autoSave; }

signals:
    void layoutChanged(const QString &preset);
    void stateSaved();
    void stateRestored();

private:
    QMainWindow *m_mainWindow{nullptr};
    QMap<QString, QDockWidget *> m_dockWidgets;
    bool m_autoSave{true};
};
