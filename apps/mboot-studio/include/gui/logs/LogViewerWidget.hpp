#pragma once

#include <QWidget>
#include <QPlainTextEdit>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <memory>

class LogViewerWidget : public QWidget {
    Q_OBJECT
public:
    explicit LogViewerWidget(QWidget *parent = nullptr);
    void appendLog(const QString &message, const QString &level = "info");
    void clear();
    void setFilter(const QString &filter);
    QString exportText() const;
    QString exportCsv() const;
    QString exportJson() const;

signals:
    void logCleared();

private:
    void setupUi();
    QPlainTextEdit *m_logView;
    QLineEdit *m_searchBox;
    QComboBox *m_levelFilter, *m_sourceFilter;
    QPushButton *m_clearBtn, *m_exportBtn;
};
