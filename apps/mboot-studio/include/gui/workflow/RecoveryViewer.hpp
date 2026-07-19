#pragma once

#include <QWidget>
#include <QTableWidget>
#include <memory>

class RecoveryViewer : public QWidget {
    Q_OBJECT
public:
    explicit RecoveryViewer(QWidget *parent = nullptr);
    void setRecoveryPlan(const QVariantList &plan);
    void clear();

private:
    void setupUi();
    QTableWidget *m_table;
};
