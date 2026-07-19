#include "gui/flash/FlashStatistics.hpp"
#include <QFormLayout>

FlashStatistics::FlashStatistics(QWidget *parent)
    : QWidget(parent)
{
    setupUi();
}

void FlashStatistics::setBytesTransferred(qint64 bytes)
{
    m_bytesLabel->setText(tr("%1 bytes").arg(bytes));
}

void FlashStatistics::setTransferSpeed(double bps)
{
    m_speedLabel->setText(tr("%1 B/s").arg(bps, 0, 'f', 2));
}

void FlashStatistics::setRetryCount(int count)
{
    m_retryLabel->setText(QString::number(count));
}

void FlashStatistics::setErrorCount(int count)
{
    m_errorsLabel->setText(QString::number(count));
}

void FlashStatistics::setWarningCount(int count)
{
    m_warningsLabel->setText(QString::number(count));
}

void FlashStatistics::setElapsed(qint64 ms)
{
    m_elapsedLabel->setText(tr("%1 ms").arg(ms));
}

void FlashStatistics::reset()
{
    setBytesTransferred(0);
    setTransferSpeed(0.0);
    setRetryCount(0);
    setErrorCount(0);
    setWarningCount(0);
    setElapsed(0);
}

void FlashStatistics::setupUi()
{
    auto *form = new QFormLayout(this);
    m_bytesLabel = new QLabel("0", this);
    m_speedLabel = new QLabel("0 B/s", this);
    m_retryLabel = new QLabel("0", this);
    m_errorsLabel = new QLabel("0", this);
    m_warningsLabel = new QLabel("0", this);
    m_elapsedLabel = new QLabel("0 ms", this);

    form->addRow(tr("Transferred:"), m_bytesLabel);
    form->addRow(tr("Speed:"), m_speedLabel);
    form->addRow(tr("Retries:"), m_retryLabel);
    form->addRow(tr("Errors:"), m_errorsLabel);
    form->addRow(tr("Warnings:"), m_warningsLabel);
    form->addRow(tr("Elapsed:"), m_elapsedLabel);
}
