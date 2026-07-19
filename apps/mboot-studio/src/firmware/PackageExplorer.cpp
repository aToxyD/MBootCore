#include "gui/firmware/PackageExplorer.hpp"
#include "gui/firmware/FirmwareInspector.hpp"
#include "gui/firmware/MetadataViewer.hpp"
#include "gui/firmware/PartitionViewer.hpp"
#include "gui/firmware/DependencyViewer.hpp"
#include "gui/firmware/IntegrityChecker.hpp"
#include "gui/runtime/RuntimeBridge.hpp"
#include "gui/runtime/RuntimeEventDispatcher.hpp"
#include <QVBoxLayout>
#include <QFileInfo>

PackageExplorer::PackageExplorer(QWidget *parent)
    : QWidget(parent)
{
    setupUi();
}

void PackageExplorer::setRuntimeBridge(gui::runtime::RuntimeBridge *bridge)
{
    if (m_bridge) return;
    m_bridge = bridge;

    connect(m_bridge, &gui::runtime::RuntimeBridge::packageLoaded,
            this, &PackageExplorer::onPackageLoadedFromBridge);
    connect(m_bridge, &gui::runtime::RuntimeBridge::firmwareValidated,
            this, &PackageExplorer::onFirmwareValidated);
}

void PackageExplorer::loadPackage(const QString &path)
{
    QFileInfo fi(path);
    if (!fi.exists()) {
        return;
    }

    if (m_bridge) {
        m_bridge->loadFirmwarePackage(path.toStdString());
    } else {
        m_inspector->inspect(path);
        emit packageLoaded(path);
    }
}

void PackageExplorer::closePackage()
{
    m_inspector = nullptr;
    m_metadataViewer->clear();
    m_partitionViewer->clear();
    m_dependencyViewer->clear();
    emit packageClosed();
}

void PackageExplorer::onPackageLoadedFromBridge(const QString &path, bool success)
{
    if (success) {
        QFileInfo fi(path);
        if (fi.exists()) {
            m_inspector->inspect(path);
        }
        emit packageLoaded(path);
    }
}

void PackageExplorer::onFirmwareValidated(bool valid, const QString &message)
{
    Q_UNUSED(valid)
    Q_UNUSED(message)
}

void PackageExplorer::setupUi()
{
    auto *layout = new QVBoxLayout(this);

    m_splitter = new QSplitter(Qt::Horizontal, this);
    m_splitter->setAccessibleName("Package Explorer Splitter");
    m_splitter->setAccessibleDescription("Split view showing package tree and detail tabs");

    m_treeView = new QTreeView(this);
    m_splitter->addWidget(m_treeView);

    m_inspector = new FirmwareInspector(this);
    m_metadataViewer = new MetadataViewer(this);
    m_partitionViewer = new PartitionViewer(this);
    m_dependencyViewer = new DependencyViewer(this);
    m_integrityChecker = new IntegrityChecker(this);

    m_treeView->setAccessibleName("Package Tree");
    m_treeView->setAccessibleDescription("View firmware package contents in a tree structure");
    m_inspector->setAccessibleName("Firmware Inspector");
    m_inspector->setAccessibleDescription("Inspect firmware image details and properties");
    m_metadataViewer->setAccessibleName("Metadata Viewer");
    m_metadataViewer->setAccessibleDescription("View firmware package metadata");
    m_partitionViewer->setAccessibleName("Partition Viewer");
    m_partitionViewer->setAccessibleDescription("View partition layout and sizes");
    m_dependencyViewer->setAccessibleName("Dependency Viewer");
    m_dependencyViewer->setAccessibleDescription("View firmware dependencies");
    m_integrityChecker->setAccessibleName("Integrity Checker");
    m_integrityChecker->setAccessibleDescription("Verify firmware package integrity and checksums");

    auto *rightTabs = new QTabWidget(this);
    rightTabs->setAccessibleName("Package Details");
    rightTabs->setAccessibleDescription("Detailed views of the loaded firmware package");
    rightTabs->addTab(m_inspector, tr("Inspector"));
    rightTabs->addTab(m_metadataViewer, tr("Metadata"));
    rightTabs->addTab(m_partitionViewer, tr("Partitions"));
    rightTabs->addTab(m_dependencyViewer, tr("Dependencies"));
    rightTabs->addTab(m_integrityChecker, tr("Integrity"));
    m_splitter->addWidget(rightTabs);

    layout->addWidget(m_splitter);
}
