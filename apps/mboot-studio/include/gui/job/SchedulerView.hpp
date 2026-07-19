#pragma once

#include <QWidget>
#include <QTableWidget>
#include <memory>

class SchedulerView : public QWidget {
    Q_OBJECT
public:
    explicit SchedulerView(QWidget *parent = nullptr);
    void setSchedule(const QVariantList &schedule);
    void clear();

private:
    void setupUi();
    QTableWidget *m_table;
};
