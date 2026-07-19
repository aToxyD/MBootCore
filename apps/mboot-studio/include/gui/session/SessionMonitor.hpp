#pragma once

#include <QWidget>
#include <QPlainTextEdit>
#include <QLineEdit>
#include <QComboBox>
#include <memory>

class SessionMonitor : public QWidget {
    Q_OBJECT
public:
    explicit SessionMonitor(QWidget *parent = nullptr);
    void appendLog(const QString &message, const QString &level = "info");
    void clear();
    void setAutoScroll(bool enabled) { m_autoScroll = enabled; }

signals:
    void logEntryAdded(const QString &entry);

private slots:
    void onFilterChanged(int index);
    void onSearchChanged(const QString &text);

private:
    void setupUi();
    QPlainTextEdit *m_logView;
    QLineEdit *m_searchBox;
    QComboBox *m_filterCombo;
    bool m_autoScroll{true};
};
