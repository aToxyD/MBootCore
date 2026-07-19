#include "gui/framework/WindowStateManager.hpp"
#include "gui/framework/SettingsManager.hpp"
#include <QMainWindow>
#include <QDockWidget>

WindowStateManager::WindowStateManager(QObject *parent) : QObject(parent) {}

void WindowStateManager::registerMainWindow(QMainWindow *window)
{
    m_mainWindow = window;
}

void WindowStateManager::registerDockWidget(const QString &name, QDockWidget *dock)
{
    m_dockWidgets[name] = dock;
}

void WindowStateManager::saveState()
{
    if (!m_mainWindow) return;
    auto *settings = qobject_cast<SettingsManager *>(parent());
    if (settings) {
        settings->setWindowGeometry(m_mainWindow->saveGeometry());
        settings->setWindowState(m_mainWindow->saveState());
    }
    emit stateSaved();
}

void WindowStateManager::restoreState()
{
    if (!m_mainWindow) return;
    auto *settings = qobject_cast<SettingsManager *>(parent());
    if (settings) {
        auto geo = settings->windowGeometry();
        if (!geo.isEmpty()) m_mainWindow->restoreGeometry(geo);
        auto state = settings->windowState();
        if (!state.isEmpty()) m_mainWindow->restoreState(state);
    }
    emit stateRestored();
}

void WindowStateManager::saveLayout(const QString &preset)
{
    if (!m_mainWindow) return;
    auto *settings = qobject_cast<SettingsManager *>(parent());
    if (settings) {
        settings->setValue(QString("layouts/%1/geometry").arg(preset), m_mainWindow->saveGeometry());
        settings->setValue(QString("layouts/%1/state").arg(preset), m_mainWindow->saveState());
    }
}

void WindowStateManager::restoreLayout(const QString &preset)
{
    if (!m_mainWindow) return;
    auto *settings = qobject_cast<SettingsManager *>(parent());
    if (settings) {
        auto geo = settings->getValue(QString("layouts/%1/geometry").arg(preset)).toByteArray();
        auto state = settings->getValue(QString("layouts/%1/state").arg(preset)).toByteArray();
        if (!geo.isEmpty()) m_mainWindow->restoreGeometry(geo);
        if (!state.isEmpty()) m_mainWindow->restoreState(state);
    }
    emit layoutChanged(preset);
}

QStringList WindowStateManager::availablePresets() const
{
    return {"Default", "Development", "Debug"};
}

void WindowStateManager::resetLayout()
{
    if (!m_mainWindow) return;
    m_mainWindow->restoreState(QByteArray());
}
