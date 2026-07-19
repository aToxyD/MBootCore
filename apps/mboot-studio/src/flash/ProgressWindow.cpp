#include "gui/flash/ProgressWindow.hpp"
#include <QVBoxLayout>

ProgressWindow::ProgressWindow(QWidget *parent)
    : QWidget(parent)
{
    setupUi();
}

void ProgressWindow::setProgress(int percent)
{
    m_progressBar->setValue(percent);
}

void ProgressWindow::setStatus(const QString &status)
{
    m_statusLabel->setText(status);
}

void ProgressWindow::setOperation(const QString &op)
{
    m_operationLabel->setText(op);
}

void ProgressWindow::setEstimatedTime(qint64 ms)
{
    m_etaLabel->setText(tr("ETA: %1 ms").arg(ms));
}

void ProgressWindow::reset()
{
    m_progressBar->setValue(0);
    m_statusLabel->setText(tr("Idle"));
    m_operationLabel->setText(tr("None"));
    m_etaLabel->setText(tr("ETA: -"));
}

void ProgressWindow::setupUi()
{
    auto *layout = new QVBoxLayout(this);
    m_operationLabel = new QLabel(tr("Operation: None"), this);
    m_progressBar = new QProgressBar(this);
    m_progressBar->setRange(0, 100);
    m_statusLabel = new QLabel(tr("Idle"), this);
    m_etaLabel = new QLabel(tr("ETA: -"), this);

    layout->addWidget(m_operationLabel);
    layout->addWidget(m_progressBar);
    layout->addWidget(m_statusLabel);
    layout->addWidget(m_etaLabel);
}
