#pragma once

#include <QWidget>
#include <QTreeView>
#include <QSplitter>
#include <memory>

namespace gui::runtime {
class RuntimeBridge;
}

class FirmwareInspector;
class MetadataViewer;
class PartitionViewer;
class DependencyViewer;
class IntegrityChecker;

class PackageExplorer : public QWidget {
    Q_OBJECT
public:
    explicit PackageExplorer(QWidget *parent = nullptr);

    void setRuntimeBridge(gui::runtime::RuntimeBridge *bridge);
    void loadPackage(const QString &path);
    void closePackage();

signals:
    void packageLoaded(const QString &path);
    void packageClosed();

private slots:
    void onPackageLoadedFromBridge(const QString &path, bool success);
    void onFirmwareValidated(bool valid, const QString &message);

private:
    void setupUi();
    gui::runtime::RuntimeBridge *m_bridge{nullptr};
    QTreeView *m_treeView;
    QSplitter *m_splitter;
    FirmwareInspector *m_inspector;
    MetadataViewer *m_metadataViewer;
    PartitionViewer *m_partitionViewer;
    DependencyViewer *m_dependencyViewer;
    IntegrityChecker *m_integrityChecker;
};
