#pragma once

#include <QWidget>
#include <QTableWidget>
#include <memory>

class PartitionViewer : public QWidget {
    Q_OBJECT
public:
    explicit PartitionViewer(QWidget *parent = nullptr);
    void setPartitions(const QVariantList &partitions);
    void clear();

private:
    void setupUi();
    QTableWidget *m_table;
};
