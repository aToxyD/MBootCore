#include "gui/job/JobHistoryWidget.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>

JobHistoryWidget::JobHistoryWidget(QWidget *parent)
    : QWidget(parent)
    , m_table(new QTableWidget(this))
    , m_filterCombo(new QComboBox(this))
{
    setupUi();
}

void JobHistoryWidget::setupUi()
{
    auto *layout = new QVBoxLayout(this);

    auto *filterLayout = new QHBoxLayout();
    filterLayout->addWidget(new QLabel(tr("Filter:"), this));
    m_filterCombo->addItems({tr("All"), tr("Completed"), tr("Failed"), tr("Cancelled")});
    filterLayout->addWidget(m_filterCombo);
    layout->addLayout(filterLayout);

    layout->addWidget(m_table);

    m_table->setColumnCount(4);
    m_table->setHorizontalHeaderLabels({tr("ID"), tr("Type"), tr("Status"), tr("Time")});
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);

    connect(m_filterCombo, &QComboBox::currentTextChanged, this, [this](const QString &text) {
        setFilter(text);
    });
}

void JobHistoryWidget::addEntry(const QVariantMap &entry)
{
    int row = m_table->rowCount();
    m_table->insertRow(row);
    m_table->setItem(row, 0, new QTableWidgetItem(entry.value("id").toString()));
    m_table->setItem(row, 1, new QTableWidgetItem(entry.value("type").toString()));
    m_table->setItem(row, 2, new QTableWidgetItem(entry.value("status").toString()));
    m_table->setItem(row, 3, new QTableWidgetItem(entry.value("time").toString()));
}

void JobHistoryWidget::clear()
{
    m_table->setRowCount(0);
}

void JobHistoryWidget::setFilter(const QString &status)
{
    Q_UNUSED(status)
    for (int i = 0; i < m_table->rowCount(); ++i)
        m_table->setRowHidden(i, false);
}
