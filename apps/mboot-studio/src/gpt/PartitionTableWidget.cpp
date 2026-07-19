#include "gui/gpt/PartitionTableWidget.hpp"
#include "gui/gpt/GPTViewer.hpp"
#include "gui/gpt/PartitionEditor.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>

PartitionTableWidget::PartitionTableWidget(QWidget *parent)
    : QWidget(parent)
{
    setupUi();
}

void PartitionTableWidget::loadTable(const QString &path)
{
    Q_UNUSED(path)
}

void PartitionTableWidget::saveTable(const QString &path)
{
    Q_UNUSED(path)
}

void PartitionTableWidget::setupUi()
{
    auto *mainLayout = new QVBoxLayout(this);

    m_splitter = new QSplitter(Qt::Horizontal, this);
    m_gptViewer = new GPTViewer(this);
    m_editor = new PartitionEditor(this);
    m_gptViewer->setAccessibleName("GPT Table Viewer");
    m_gptViewer->setAccessibleDescription("View the GUID Partition Table structure");
    m_editor->setAccessibleName("Partition Editor");
    m_editor->setAccessibleDescription("Edit partition table entries");
    m_splitter->setAccessibleName("GPT Splitter");
    m_splitter->setAccessibleDescription("Split view showing GPT table and editor");

    m_splitter->addWidget(m_gptViewer);
    m_splitter->addWidget(m_editor);
    mainLayout->addWidget(m_splitter);

    auto *btnLayout = new QHBoxLayout();
    m_backupBtn = new QPushButton(tr("Backup"), this);
    m_restoreBtn = new QPushButton(tr("Restore"), this);
    m_compareBtn = new QPushButton(tr("Compare"), this);
    m_refreshBtn = new QPushButton(tr("Refresh"), this);

    m_backupBtn->setAccessibleName("Backup GPT");
    m_backupBtn->setAccessibleDescription("Backup the current partition table to a file");
    m_backupBtn->setToolTip("Save partition table to a backup file");

    m_restoreBtn->setAccessibleName("Restore GPT");
    m_restoreBtn->setAccessibleDescription("Restore partition table from a backup file");
    m_restoreBtn->setToolTip("Load and apply a partition table backup");

    m_compareBtn->setAccessibleName("Compare GPT");
    m_compareBtn->setAccessibleDescription("Compare current partition table with a reference");
    m_compareBtn->setToolTip("Compare partition tables");

    m_refreshBtn->setAccessibleName("Refresh GPT");
    m_refreshBtn->setAccessibleDescription("Refresh the partition table from the device");
    m_refreshBtn->setToolTip("Reload partition table from device");
    m_refreshBtn->setDefault(true);

    btnLayout->addWidget(m_backupBtn);
    btnLayout->addWidget(m_restoreBtn);
    btnLayout->addWidget(m_compareBtn);
    btnLayout->addWidget(m_refreshBtn);
    mainLayout->addLayout(btnLayout);

    QWidget::setTabOrder(m_backupBtn, m_restoreBtn);
    QWidget::setTabOrder(m_restoreBtn, m_compareBtn);
    QWidget::setTabOrder(m_compareBtn, m_refreshBtn);

    connect(m_backupBtn, &QPushButton::clicked, this, &PartitionTableWidget::backupRequested);
    connect(m_restoreBtn, &QPushButton::clicked, this, &PartitionTableWidget::restoreRequested);
    connect(m_compareBtn, &QPushButton::clicked, this, &PartitionTableWidget::compareRequested);
}
