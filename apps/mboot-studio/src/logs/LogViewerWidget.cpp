#include "gui/logs/LogViewerWidget.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QFileDialog>
#include <QTextStream>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>

LogViewerWidget::LogViewerWidget(QWidget *parent)
    : QWidget(parent)
    , m_logView(new QPlainTextEdit(this))
    , m_searchBox(new QLineEdit(this))
    , m_levelFilter(new QComboBox(this))
    , m_sourceFilter(new QComboBox(this))
    , m_clearBtn(new QPushButton(tr("Clear"), this))
    , m_exportBtn(new QPushButton(tr("Export"), this))
{
    setupUi();
}

void LogViewerWidget::setupUi()
{
    auto *layout = new QVBoxLayout(this);

    m_logView->setAccessibleName("Log Output");
    m_logView->setAccessibleDescription("Application log messages");
    m_logView->setPlaceholderText(tr("Log messages will appear here..."));

    m_searchBox->setAccessibleName("Log Search");
    m_searchBox->setAccessibleDescription("Search or filter log messages by text");
    m_searchBox->setPlaceholderText(tr("Search logs..."));
    m_searchBox->setToolTip(tr("Type to search log messages"));

    m_levelFilter->setAccessibleName("Log Level Filter");
    m_levelFilter->setAccessibleDescription("Filter log messages by severity level");
    m_levelFilter->setToolTip(tr("Show only logs at the selected severity level"));

    m_sourceFilter->setAccessibleName("Log Source Filter");
    m_sourceFilter->setAccessibleDescription("Filter log messages by source component");
    m_sourceFilter->setToolTip(tr("Show only logs from the selected source"));

    m_clearBtn->setAccessibleName("Clear Logs");
    m_clearBtn->setAccessibleDescription("Clear all log messages from the view");
    m_clearBtn->setToolTip(tr("Clear the log view"));

    m_exportBtn->setAccessibleName("Export Logs");
    m_exportBtn->setAccessibleDescription("Export log messages to a file (text, CSV, or JSON)");
    m_exportBtn->setToolTip(tr("Export logs to a file"));

    auto *filterLayout = new QHBoxLayout();
    auto *searchLabel = new QLabel(tr("Search:"), this);
    searchLabel->setAccessibleName("Search Label");
    filterLayout->addWidget(searchLabel);
    filterLayout->addWidget(m_searchBox);
    auto *levelLabel = new QLabel(tr("Level:"), this);
    levelLabel->setAccessibleName("Level Label");
    filterLayout->addWidget(levelLabel);
    m_levelFilter->addItems({tr("All"), tr("Debug"), tr("Info"), tr("Warning"), tr("Error")});
    filterLayout->addWidget(m_levelFilter);
    auto *sourceLabel = new QLabel(tr("Source:"), this);
    sourceLabel->setAccessibleName("Source Label");
    filterLayout->addWidget(sourceLabel);
    m_sourceFilter->addItems({tr("All"), tr("System"), tr("Protocol"), tr("Transport")});
    filterLayout->addWidget(m_sourceFilter);
    filterLayout->addStretch();
    filterLayout->addWidget(m_clearBtn);
    filterLayout->addWidget(m_exportBtn);
    layout->addLayout(filterLayout);

    m_logView->setReadOnly(true);
    m_logView->setFont(QFont("Consolas", 9));
    layout->addWidget(m_logView);

    connect(m_clearBtn, &QPushButton::clicked, this, &LogViewerWidget::clear);
    connect(m_searchBox, &QLineEdit::textChanged, this, &LogViewerWidget::setFilter);
    connect(m_exportBtn, &QPushButton::clicked, this, [this]() {
        QString path = QFileDialog::getSaveFileName(this, tr("Export Logs"),
            QString(), tr("Text Files (*.txt);;CSV Files (*.csv);;JSON Files (*.json);;All Files (*)"));
        if (!path.isEmpty()) {
            QFile file(path);
            if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
                if (path.endsWith(".json"))
                    file.write(exportJson().toUtf8());
                else if (path.endsWith(".csv"))
                    file.write(exportCsv().toUtf8());
                else
                    file.write(exportText().toUtf8());
            }
        }
    });
}

void LogViewerWidget::appendLog(const QString &message, const QString &level)
{
    Q_UNUSED(level)
    m_logView->appendPlainText(message);
}

void LogViewerWidget::clear()
{
    m_logView->clear();
    emit logCleared();
}

void LogViewerWidget::setFilter(const QString &filter)
{
    Q_UNUSED(filter)
}

QString LogViewerWidget::exportText() const
{
    return m_logView->toPlainText();
}

QString LogViewerWidget::exportCsv() const
{
    QString csv = "Level,Message\n";
    for (const auto &line : m_logView->toPlainText().split('\n'))
        csv += "Info," + line + "\n";
    return csv;
}

QString LogViewerWidget::exportJson() const
{
    QJsonArray arr;
    for (const auto &line : m_logView->toPlainText().split('\n')) {
        QJsonObject obj;
        obj["level"] = "info";
        obj["message"] = line;
        arr.append(obj);
    }
    QJsonDocument doc(arr);
    return QString::fromUtf8(doc.toJson(QJsonDocument::Indented));
}
