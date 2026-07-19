#pragma once

#include <QWidget>
#include <QTableWidget>
#include <QCheckBox>
#include <memory>

class PartitionSelection : public QWidget {
    Q_OBJECT
public:
    explicit PartitionSelection(QWidget *parent = nullptr);
    void setPartitions(const QVariantList &partitions);
    QStringList selectedPartitions() const;
    void selectAll(bool selected);

signals:
    void selectionChanged(const QStringList &selected);

private:
    void setupUi();
    QTableWidget *m_table;
    QCheckBox *m_selectAllCheck;
};
