#include "gui/job/RecoveryPolicyEditor.hpp"
#include <QVBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QGroupBox>

RecoveryPolicyEditor::RecoveryPolicyEditor(QWidget *parent)
    : QWidget(parent)
    , m_maxRetries(new QSpinBox(this))
    , m_autoRollback(new QCheckBox(tr("Auto Rollback"), this))
    , m_notifyOnFailure(new QCheckBox(tr("Notify on Failure"), this))
    , m_failureAction(new QComboBox(this))
    , m_applyBtn(new QPushButton(tr("Apply"), this))
{
    setupUi();
}

void RecoveryPolicyEditor::setupUi()
{
    auto *layout = new QVBoxLayout(this);

    auto *group = new QGroupBox(tr("Recovery Policy"), this);
    auto *form = new QFormLayout(group);

    m_maxRetries->setRange(0, 10);
    m_maxRetries->setValue(3);
    form->addRow(tr("Max Retries"), m_maxRetries);

    m_failureAction->addItems({tr("Abort"), tr("Retry"), tr("Skip"), tr("Rollback")});
    form->addRow(tr("Failure Action"), m_failureAction);

    form->addRow(m_autoRollback);
    form->addRow(m_notifyOnFailure);

    layout->addWidget(group);
    layout->addWidget(m_applyBtn);
    layout->addStretch();

    connect(m_applyBtn, &QPushButton::clicked, this, [this]() {
        emit policyChanged(policy());
    });
}

QVariantMap RecoveryPolicyEditor::policy() const
{
    QVariantMap p;
    p["maxRetries"] = m_maxRetries->value();
    p["failureAction"] = m_failureAction->currentText();
    p["autoRollback"] = m_autoRollback->isChecked();
    p["notifyOnFailure"] = m_notifyOnFailure->isChecked();
    return p;
}

void RecoveryPolicyEditor::setPolicy(const QVariantMap &policy)
{
    if (policy.contains("maxRetries"))
        m_maxRetries->setValue(policy["maxRetries"].toInt());
    if (policy.contains("failureAction"))
        m_failureAction->setCurrentText(policy["failureAction"].toString());
    if (policy.contains("autoRollback"))
        m_autoRollback->setChecked(policy["autoRollback"].toBool());
    if (policy.contains("notifyOnFailure"))
        m_notifyOnFailure->setChecked(policy["notifyOnFailure"].toBool());
}
