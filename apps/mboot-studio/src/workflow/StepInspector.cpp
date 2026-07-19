#include "gui/workflow/StepInspector.hpp"
#include <QVBoxLayout>
#include <QFormLayout>

StepInspector::StepInspector(QWidget *parent)
    : QWidget(parent)
    , m_nameLabel(new QLabel(tr("Name: --"), this))
    , m_typeLabel(new QLabel(tr("Type: --"), this))
    , m_statusLabel(new QLabel(tr("Status: --"), this))
    , m_detailsView(new QTextEdit(this))
{
    setupUi();
}

void StepInspector::setupUi()
{
    auto *layout = new QVBoxLayout(this);
    auto *form = new QFormLayout();
    form->addRow(tr("Name"), m_nameLabel);
    form->addRow(tr("Type"), m_typeLabel);
    form->addRow(tr("Status"), m_statusLabel);
    layout->addLayout(form);

    m_detailsView->setReadOnly(true);
    layout->addWidget(new QLabel(tr("Details"), this));
    layout->addWidget(m_detailsView);
}

void StepInspector::inspectStep(const QVariantMap &step)
{
    m_nameLabel->setText(step.value("name", "--").toString());
    m_typeLabel->setText(step.value("type", "--").toString());
    m_statusLabel->setText(step.value("status", "--").toString());
    m_detailsView->setPlainText(step.value("details").toString());
}

void StepInspector::clear()
{
    m_nameLabel->setText(tr("Name: --"));
    m_typeLabel->setText(tr("Type: --"));
    m_statusLabel->setText(tr("Status: --"));
    m_detailsView->clear();
}
