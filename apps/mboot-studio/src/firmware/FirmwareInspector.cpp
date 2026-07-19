#include "gui/firmware/FirmwareInspector.hpp"
#include <QFormLayout>
#include <QVBoxLayout>

FirmwareInspector::FirmwareInspector(QWidget *parent)
    : QWidget(parent)
{
    setupUi();
}

void FirmwareInspector::inspect(const QString &path)
{
    Q_UNUSED(path)
}

void FirmwareInspector::setupUi()
{
    auto *layout = new QVBoxLayout(this);
    auto *form = new QFormLayout();

    m_nameLabel = new QLabel(tr("-"), this);
    m_versionLabel = new QLabel(tr("-"), this);
    m_sizeLabel = new QLabel(tr("-"), this);
    m_typeLabel = new QLabel(tr("-"), this);
    m_hashLabel = new QLabel(tr("-"), this);
    m_signatureLabel = new QLabel(tr("-"), this);
    m_compressionLabel = new QLabel(tr("-"), this);

    form->addRow(tr("Name:"), m_nameLabel);
    form->addRow(tr("Version:"), m_versionLabel);
    form->addRow(tr("Size:"), m_sizeLabel);
    form->addRow(tr("Type:"), m_typeLabel);
    form->addRow(tr("Hash:"), m_hashLabel);
    form->addRow(tr("Signature:"), m_signatureLabel);
    form->addRow(tr("Compression:"), m_compressionLabel);

    layout->addLayout(form);
    layout->addStretch();
}
