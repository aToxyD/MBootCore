#include "gui/gpt/CompareDialog.hpp"
#include <QVBoxLayout>
#include <QHeaderView>

CompareDialog::CompareDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Compare GPT Tables"));
    setupUi();
}

void CompareDialog::compare(const QString &path1, const QString &path2)
{
    Q_UNUSED(path1)
    Q_UNUSED(path2)
}

void CompareDialog::setupUi()
{
    auto *layout = new QVBoxLayout(this);
    m_diffTable = new QTableWidget(0, 5, this);
    m_diffTable->setHorizontalHeaderLabels({
        tr("Partition"), tr("Attribute"), tr("File 1"), tr("File 2"), tr("Status")
    });
    m_diffTable->horizontalHeader()->setStretchLastSection(true);
    m_diffTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_diffTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    layout->addWidget(m_diffTable);
}
