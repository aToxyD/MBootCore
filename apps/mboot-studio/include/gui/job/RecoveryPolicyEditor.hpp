#pragma once

#include <QWidget>
#include <QSpinBox>
#include <QCheckBox>
#include <QComboBox>
#include <QPushButton>
#include <memory>

class RecoveryPolicyEditor : public QWidget {
    Q_OBJECT
public:
    explicit RecoveryPolicyEditor(QWidget *parent = nullptr);
    QVariantMap policy() const;
    void setPolicy(const QVariantMap &policy);

signals:
    void policyChanged(const QVariantMap &policy);

private:
    void setupUi();
    QSpinBox *m_maxRetries;
    QCheckBox *m_autoRollback, *m_notifyOnFailure;
    QComboBox *m_failureAction;
    QPushButton *m_applyBtn;
};
