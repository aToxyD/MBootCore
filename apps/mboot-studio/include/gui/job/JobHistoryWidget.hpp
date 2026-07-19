#pragma once

#include <QWidget>
#include <QTableWidget>
#include <QComboBox>
#include <memory>

class JobHistoryWidget : public QWidget {
    Q_OBJECT
public:
    explicit JobHistoryWidget(QWidget *parent = nullptr);
    void addEntry(const QVariantMap &entry);
    void clear();
    void setFilter(const QString &status);

private:
    void setupUi();
    QTableWidget *m_table;
    QComboBox *m_filterCombo;
};
