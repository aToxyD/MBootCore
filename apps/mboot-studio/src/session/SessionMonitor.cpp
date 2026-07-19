#include "gui/session/SessionMonitor.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>

SessionMonitor::SessionMonitor(QWidget *parent)
    : QWidget(parent)
{
    setupUi();
}

void SessionMonitor::appendLog(const QString &message, const QString &level)
{
    Q_UNUSED(level)
    m_logView->appendPlainText(message);
    if (m_autoScroll) {
        auto cursor = m_logView->textCursor();
        cursor.movePosition(QTextCursor::End);
        m_logView->setTextCursor(cursor);
    }
    emit logEntryAdded(message);
}

void SessionMonitor::clear()
{
    m_logView->clear();
}

void SessionMonitor::onFilterChanged(int index)
{
    Q_UNUSED(index)
}

void SessionMonitor::onSearchChanged(const QString &text)
{
    Q_UNUSED(text)
}

void SessionMonitor::setupUi()
{
    auto *layout = new QVBoxLayout(this);

    auto *topBar = new QHBoxLayout();
    m_filterCombo = new QComboBox(this);
    m_filterCombo->addItems({tr("All"), tr("Info"), tr("Warning"), tr("Error"), tr("Debug")});
    topBar->addWidget(m_filterCombo);

    m_searchBox = new QLineEdit(this);
    m_searchBox->setPlaceholderText(tr("Search logs..."));
    topBar->addWidget(m_searchBox);
    layout->addLayout(topBar);

    m_logView = new QPlainTextEdit(this);
    m_logView->setReadOnly(true);
    m_logView->setMaximumBlockCount(10000);
    layout->addWidget(m_logView);

    connect(m_filterCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &SessionMonitor::onFilterChanged);
    connect(m_searchBox, &QLineEdit::textChanged,
            this, &SessionMonitor::onSearchChanged);
}
