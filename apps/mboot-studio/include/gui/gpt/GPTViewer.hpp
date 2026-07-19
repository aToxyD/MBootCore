#pragma once

#include <QWidget>
#include <QTableWidget>
#include <QComboBox>
#include <memory>

class GPTViewer : public QWidget {
    Q_OBJECT
public:
    explicit GPTViewer(QWidget *parent = nullptr);
    void setPartitions(const QVariantList &partitions);

signals:
    void partitionSelected(int index);

private:
    void setupUi();
    QTableWidget *m_table;
    QComboBox *m_diskCombo;
};
