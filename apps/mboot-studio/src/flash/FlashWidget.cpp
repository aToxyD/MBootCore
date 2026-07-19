#include "gui/flash/FlashWidget.hpp"
#include "gui/flash/FlashPlanViewer.hpp"
#include "gui/flash/PartitionSelection.hpp"
#include "gui/flash/ProgressWindow.hpp"
#include "gui/flash/FlashStatistics.hpp"
#include "gui/runtime/RuntimeBridge.hpp"
#include "gui/runtime/RuntimeEventDispatcher.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>

FlashWidget::FlashWidget(QWidget *parent)
    : QWidget(parent)
{
    setupUi();
}

void FlashWidget::setRuntimeBridge(gui::runtime::RuntimeBridge *bridge)
{
    if (m_bridge) return;
    m_bridge = bridge;

    auto *dispatcher = m_bridge->eventDispatcher();
    connect(dispatcher, &gui::runtime::RuntimeEventDispatcher::flashOperationChanged,
            this, &FlashWidget::onFlashOperationChanged);
    connect(dispatcher, &gui::runtime::RuntimeEventDispatcher::flashProgressChanged,
            this, &FlashWidget::onFlashProgressChanged);
    connect(dispatcher, &gui::runtime::RuntimeEventDispatcher::flashResult,
            this, &FlashWidget::onFlashCompleted);
    connect(m_bridge, &gui::runtime::RuntimeBridge::flashCancelled,
            this, &FlashWidget::onFlashCancelled);
    connect(m_bridge, &gui::runtime::RuntimeBridge::flashFailed,
            this, &FlashWidget::onFlashFailed);
    connect(m_bridge, &gui::runtime::RuntimeBridge::packageLoaded,
            this, &FlashWidget::onPackageLoaded);

    updateButtonStates(gui::runtime::FlashStatus::Idle);
}

void FlashWidget::loadPlan(const QString &path)
{
    if (m_bridge) {
        m_bridge->loadFirmwarePackage(path.toStdString());
    }
}

void FlashWidget::startFlash()
{
    if (m_bridge) {
        m_bridge->startFlash();
    }
}

void FlashWidget::stopFlash()
{
    if (m_bridge) {
        m_bridge->cancelFlash();
    }
}

void FlashWidget::pauseFlash()
{
}

void FlashWidget::resumeFlash()
{
}

void FlashWidget::onFlashOperationChanged(const gui::runtime::FlashOperationView &op)
{
    updateButtonStates(op.status);

    if (op.packageName.empty()) {
        m_statusLabel->setText(tr("No package loaded"));
    } else {
        m_statusLabel->setText(tr("Package: %1 (%2 images)")
            .arg(QString::fromStdString(op.packageName))
            .arg(op.imageCount));
    }
}

void FlashWidget::onFlashProgressChanged(const gui::runtime::FlashProgressView &progress)
{
    m_progressWindow->setProgress(static_cast<int>(progress.percentage));
    m_progressWindow->setStatus(QString::fromStdString(progress.stageName));
    m_progressWindow->setOperation(QString::fromStdString(progress.currentOperation));
    m_progressWindow->setEstimatedTime(progress.etaMs);

    m_statistics->setBytesTransferred(static_cast<qint64>(progress.transferredBytes));
    m_statistics->setTransferSpeed(progress.speedBps);
    m_statistics->setElapsed(progress.elapsedMs);

    emit flashProgress(static_cast<int>(progress.percentage));
}

void FlashWidget::onFlashCompleted(const gui::runtime::FlashResultView &result)
{
    updateButtonStates(gui::runtime::FlashStatus::Completed);
    m_statusLabel->setText(result.success ? tr("Flash completed successfully") : tr("Flash failed"));
    m_progressWindow->setProgress(result.success ? 100 : 0);
    emit flashCompleted(result.success);
}

void FlashWidget::onFlashCancelled()
{
    updateButtonStates(gui::runtime::FlashStatus::Idle);
    m_statusLabel->setText(tr("Flash cancelled"));
    m_progressWindow->reset();
}

void FlashWidget::onFlashFailed(const QString &error)
{
    updateButtonStates(gui::runtime::FlashStatus::Failed);
    m_statusLabel->setText(tr("Flash failed: %1").arg(error));
    emit flashError(error);
}

void FlashWidget::onPackageLoaded(const QString &path, bool success)
{
    if (success) {
        m_statusLabel->setText(tr("Package loaded: %1").arg(path));
        m_startBtn->setEnabled(true);
    } else {
        m_statusLabel->setText(tr("Failed to load package: %1").arg(path));
        m_startBtn->setEnabled(false);
    }
}

void FlashWidget::updateButtonStates(gui::runtime::FlashStatus status)
{
    using S = gui::runtime::FlashStatus;
    switch (status) {
        case S::Idle:
            m_startBtn->setEnabled(true);
            m_stopBtn->setEnabled(false);
            m_pauseBtn->setEnabled(false);
            m_resumeBtn->setEnabled(false);
            break;
        case S::Preparing:
        case S::Validating:
        case S::Verifying:
        case S::Flashing:
            m_startBtn->setEnabled(false);
            m_stopBtn->setEnabled(true);
            m_pauseBtn->setEnabled(false);
            m_resumeBtn->setEnabled(false);
            break;
        case S::Completed:
        case S::Cancelled:
        case S::Failed:
            m_startBtn->setEnabled(true);
            m_stopBtn->setEnabled(false);
            m_pauseBtn->setEnabled(false);
            m_resumeBtn->setEnabled(false);
            break;
    }
}

void FlashWidget::setupUi()
{
    auto *mainLayout = new QVBoxLayout(this);

    auto *splitter = new QSplitter(Qt::Vertical, this);
    splitter->setAccessibleName("Flash Panels");
    splitter->setAccessibleDescription("Flash plan, partition selection, progress, and statistics panels");
    m_planViewer = new FlashPlanViewer(this);
    m_partitionSelection = new PartitionSelection(this);
    m_progressWindow = new ProgressWindow(this);
    m_statistics = new FlashStatistics(this);

    splitter->addWidget(m_planViewer);
    splitter->addWidget(m_partitionSelection);
    splitter->addWidget(m_progressWindow);
    splitter->addWidget(m_statistics);
    mainLayout->addWidget(splitter);

    m_planViewer->setAccessibleName("Flash Plan Viewer");
    m_planViewer->setAccessibleDescription("View the flash plan and image layout");
    m_partitionSelection->setAccessibleName("Partition Selection");
    m_partitionSelection->setAccessibleDescription("Select which partitions to flash");
    m_progressWindow->setAccessibleName("Flash Progress");
    m_progressWindow->setAccessibleDescription("Shows flash operation progress and status");
    m_statistics->setAccessibleName("Flash Statistics");
    m_statistics->setAccessibleDescription("Shows transfer speed, bytes transferred, and timing");

    auto *btnLayout = new QHBoxLayout();
    m_startBtn = new QPushButton(tr("Start"), this);
    m_stopBtn = new QPushButton(tr("Stop"), this);
    m_pauseBtn = new QPushButton(tr("Pause"), this);
    m_resumeBtn = new QPushButton(tr("Resume"), this);

    m_startBtn->setAccessibleName("Start Flash");
    m_startBtn->setAccessibleDescription("Begin flashing firmware to the connected device");
    m_startBtn->setToolTip("Start the flash operation (load a package first)");
    m_startBtn->setDefault(true);

    m_stopBtn->setAccessibleName("Stop Flash");
    m_stopBtn->setAccessibleDescription("Cancel the current flash operation");
    m_stopBtn->setToolTip("Stop the flash operation in progress");

    m_pauseBtn->setAccessibleName("Pause Flash");
    m_pauseBtn->setAccessibleDescription("Pause the current flash operation");
    m_pauseBtn->setToolTip("Pause the flash operation (currently not supported)");

    m_resumeBtn->setAccessibleName("Resume Flash");
    m_resumeBtn->setAccessibleDescription("Resume a paused flash operation");
    m_resumeBtn->setToolTip("Resume the paused flash operation (currently not supported)");

    btnLayout->addWidget(m_startBtn);
    btnLayout->addWidget(m_stopBtn);
    btnLayout->addWidget(m_pauseBtn);
    btnLayout->addWidget(m_resumeBtn);
    mainLayout->addLayout(btnLayout);

    m_statusLabel = new QLabel(tr("Ready"), this);
    m_statusLabel->setAccessibleName("Flash Status");
    m_statusLabel->setAccessibleDescription("Current flash operation status message");
    mainLayout->addWidget(m_statusLabel);

    QWidget::setTabOrder(m_startBtn, m_stopBtn);
    QWidget::setTabOrder(m_stopBtn, m_pauseBtn);
    QWidget::setTabOrder(m_pauseBtn, m_resumeBtn);

    connect(m_startBtn, &QPushButton::clicked, this, &FlashWidget::startFlash);
    connect(m_stopBtn, &QPushButton::clicked, this, &FlashWidget::stopFlash);
    connect(m_pauseBtn, &QPushButton::clicked, this, &FlashWidget::pauseFlash);
    connect(m_resumeBtn, &QPushButton::clicked, this, &FlashWidget::resumeFlash);
}
