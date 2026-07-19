#include "gui/session/SessionStatistics.hpp"
#include <QFormLayout>
#include <QVBoxLayout>

SessionStatistics::SessionStatistics(QWidget *parent)
    : QWidget(parent)
{
    setupUi();
}

void SessionStatistics::setBytesRead(qint64 bytes)
{
    m_bytesReadLabel->setText(tr("%1 bytes").arg(bytes));
}

void SessionStatistics::setBytesWritten(qint64 bytes)
{
    m_bytesWrittenLabel->setText(tr("%1 bytes").arg(bytes));
}

void SessionStatistics::setTransferSpeed(double bps)
{
    m_speedLabel->setText(tr("%1 B/s").arg(bps, 0, 'f', 2));
}

void SessionStatistics::setElapsed(qint64 ms)
{
    m_elapsedLabel->setText(tr("%1 ms").arg(ms));
}

void SessionStatistics::setErrors(int count)
{
    m_errorsLabel->setText(QString::number(count));
}

void SessionStatistics::setWarnings(int count)
{
    m_warningsLabel->setText(QString::number(count));
}

void SessionStatistics::reset()
{
    setBytesRead(0);
    setBytesWritten(0);
    setTransferSpeed(0.0);
    setElapsed(0);
    setErrors(0);
    setWarnings(0);
}

void SessionStatistics::setupUi()
{
    auto *form = new QFormLayout(this);
    m_bytesReadLabel = new QLabel("0", this);
    m_bytesWrittenLabel = new QLabel("0", this);
    m_speedLabel = new QLabel("0 B/s", this);
    m_elapsedLabel = new QLabel("0 ms", this);
    m_errorsLabel = new QLabel("0", this);
    m_warningsLabel = new QLabel("0", this);

    form->addRow(tr("Bytes Read:"), m_bytesReadLabel);
    form->addRow(tr("Bytes Written:"), m_bytesWrittenLabel);
    form->addRow(tr("Speed:"), m_speedLabel);
    form->addRow(tr("Elapsed:"), m_elapsedLabel);
    form->addRow(tr("Errors:"), m_errorsLabel);
    form->addRow(tr("Warnings:"), m_warningsLabel);
}
