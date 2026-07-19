#include <QTest>
#include <QApplication>
#include <QSignalSpy>
#include "gui/firmware/PackageExplorer.hpp"
#include "gui/firmware/FirmwareInspector.hpp"
#include "gui/firmware/MetadataViewer.hpp"
#include "gui/firmware/PartitionViewer.hpp"
#include "gui/firmware/DependencyViewer.hpp"
#include "gui/firmware/IntegrityChecker.hpp"

class FirmwareTest : public QObject {
    Q_OBJECT
private slots:
    void testPackageExplorer();
    void testFirmwareInspector();
    void testMetadataViewer();
    void testPartitionViewer();
    void testDependencyViewer();
    void testIntegrityChecker();
};

void FirmwareTest::testPackageExplorer()
{
    PackageExplorer explorer;
    QVERIFY(explorer.width() > 0);
}

void FirmwareTest::testFirmwareInspector()
{
    FirmwareInspector inspector;
    inspector.inspect("/path/to/firmware.bin");
}

void FirmwareTest::testMetadataViewer()
{
    MetadataViewer viewer;
    QVariantMap meta;
    meta["name"] = "TestFW";
    meta["version"] = "1.0";
    viewer.setMetadata(meta);
    viewer.clear();
}

void FirmwareTest::testPartitionViewer()
{
    PartitionViewer viewer;
    QVariantList parts;
    QVariantMap p;
    p["name"] = "boot";
    p["size"] = 4096;
    parts << p;
    viewer.setPartitions(parts);
    viewer.clear();
}

void FirmwareTest::testDependencyViewer()
{
    DependencyViewer viewer;
    QVariantMap deps;
    deps["dep1"] = "1.0";
    viewer.setDependencies(deps);
    viewer.clear();
}

void FirmwareTest::testIntegrityChecker()
{
    IntegrityChecker checker;
    QSignalSpy spy(&checker, &IntegrityChecker::verificationComplete);
    checker.verify("/path/to/firmware.bin");
}

QTEST_MAIN(FirmwareTest)
#include "firmware_test.moc"
