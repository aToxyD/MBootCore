#include "gui/gpt/PartitionEditor.hpp"
#include <QFormLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>

PartitionEditor::PartitionEditor(QWidget *parent)
    : QWidget(parent)
{
    setupUi();
}

void PartitionEditor::loadPartition(const QVariantMap &partition)
{
    m_nameEdit->setText(partition.value("name").toString());
    m_guidEdit->setText(partition.value("guid").toString());
    m_startSector->setValue(static_cast<int>(partition.value("start").toLongLong()));
    m_endSector->setValue(static_cast<int>(partition.value("end").toLongLong()));
    int idx = m_typeCombo->findText(partition.value("type").toString());
    if (idx >= 0) m_typeCombo->setCurrentIndex(idx);
}

void PartitionEditor::clear()
{
    m_nameEdit->clear();
    m_guidEdit->clear();
    m_startSector->setValue(0);
    m_endSector->setValue(0);
    m_typeCombo->setCurrentIndex(0);
}

void PartitionEditor::setupUi()
{
    auto *layout = new QVBoxLayout(this);
    auto *form = new QFormLayout();

    m_nameEdit = new QLineEdit(this);
    m_guidEdit = new QLineEdit(this);
    m_startSector = new QSpinBox(this);
    m_startSector->setRange(0, 999999999);
    m_endSector = new QSpinBox(this);
    m_endSector->setRange(0, 999999999);

    m_typeCombo = new QComboBox(this);
    m_typeCombo->addItems({tr("ESP"), tr("Microsoft Reserved"), tr("Microsoft Basic Data"),
                           tr("Linux Filesystem"), tr("Linux Swap"), tr("Android Metadata")});

    form->addRow(tr("Name:"), m_nameEdit);
    form->addRow(tr("GUID:"), m_guidEdit);
    form->addRow(tr("Start Sector:"), m_startSector);
    form->addRow(tr("End Sector:"), m_endSector);
    form->addRow(tr("Type:"), m_typeCombo);
    layout->addLayout(form);

    auto *btnLayout = new QHBoxLayout();
    m_applyBtn = new QPushButton(tr("Apply"), this);
    m_createBtn = new QPushButton(tr("Create"), this);
    m_deleteBtn = new QPushButton(tr("Delete"), this);
    btnLayout->addWidget(m_applyBtn);
    btnLayout->addWidget(m_createBtn);
    btnLayout->addWidget(m_deleteBtn);
    layout->addLayout(btnLayout);
    layout->addStretch();

    connect(m_applyBtn, &QPushButton::clicked, this, [this]() {
        QVariantMap part;
        part["name"] = m_nameEdit->text();
        part["guid"] = m_guidEdit->text();
        part["start"] = m_startSector->value();
        part["end"] = m_endSector->value();
        part["type"] = m_typeCombo->currentText();
        emit partitionChanged(part);
    });
    connect(m_createBtn, &QPushButton::clicked, this, &PartitionEditor::createRequested);
    connect(m_deleteBtn, &QPushButton::clicked, this, &PartitionEditor::deleteRequested);
}
