#pragma once

#include <QWidget>
#include <QLineEdit>
#include <QSpinBox>
#include <QPushButton>
#include <QComboBox>
#include <memory>

class PartitionEditor : public QWidget {
    Q_OBJECT
public:
    explicit PartitionEditor(QWidget *parent = nullptr);
    void loadPartition(const QVariantMap &partition);
    void clear();

signals:
    void partitionChanged(const QVariantMap &partition);
    void createRequested();
    void deleteRequested();

private:
    void setupUi();
    QLineEdit *m_nameEdit, *m_guidEdit;
    QSpinBox *m_startSector, *m_endSector;
    QComboBox *m_typeCombo;
    QPushButton *m_applyBtn, *m_createBtn, *m_deleteBtn;
};
