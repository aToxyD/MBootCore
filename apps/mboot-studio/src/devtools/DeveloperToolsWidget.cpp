#include "gui/devtools/DeveloperToolsWidget.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>

DeveloperToolsWidget::DeveloperToolsWidget(QWidget *parent)
    : QWidget(parent)
    , m_tabs(new QTabWidget(this))
    , m_refreshBtn(new QPushButton(tr("Refresh"), this))
{
    setupUi();
}

void DeveloperToolsWidget::setupUi()
{
    auto *layout = new QVBoxLayout(this);

    m_refreshBtn->setAccessibleName("Refresh");
    m_refreshBtn->setAccessibleDescription("Refresh developer tools data");
    m_refreshBtn->setToolTip("Refresh all developer tools information");
    m_refreshBtn->setDefault(true);
    m_tabs->setAccessibleName("Developer Tools");
    m_tabs->setAccessibleDescription("Inspector, console, profiler, and memory analysis tools");

    auto *btnLayout = new QHBoxLayout();
    btnLayout->addStretch();
    btnLayout->addWidget(m_refreshBtn);
    layout->addLayout(btnLayout);

    auto *inspectorTab = new QWidget(this);
    inspectorTab->setAccessibleName("Inspector Tab");
    m_tabs->addTab(inspectorTab, tr("Inspector"));
    auto *consoleTab = new QWidget(this);
    consoleTab->setAccessibleName("Console Tab");
    m_tabs->addTab(consoleTab, tr("Console"));
    auto *profilerTab = new QWidget(this);
    profilerTab->setAccessibleName("Profiler Tab");
    m_tabs->addTab(profilerTab, tr("Profiler"));
    auto *memoryTab = new QWidget(this);
    memoryTab->setAccessibleName("Memory Tab");
    m_tabs->addTab(memoryTab, tr("Memory"));
    layout->addWidget(m_tabs);
}
