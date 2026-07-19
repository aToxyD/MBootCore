#pragma once

#include <QWidget>
#include <QTableWidget>
#include <memory>

class RollbackViewer : public QWidget {
    Q_OBJECT
public:
    explicit RollbackViewer(QWidget *parent = nullptr);
    void setRollbackPlan(const QVariantList &plan);
    void clear();

private:
    void setupUi();
    QTableWidget *m_table;
};
