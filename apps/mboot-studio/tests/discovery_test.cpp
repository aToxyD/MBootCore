#include <QTest>
#include <QApplication>
#include <QSignalSpy>

#include "gui/discovery/DeviceListModel.hpp"
#include "gui/discovery/DeviceTreeModel.hpp"
#include "gui/discovery/DeviceFilterProxyModel.hpp"
#include "gui/discovery/DeviceDetailsWidget.hpp"
#include "gui/discovery/DevicePropertiesWidget.hpp"

class DiscoveryTest : public QObject {
    Q_OBJECT

private slots:
    void testDeviceListModel();
    void testDeviceTreeModel();
    void testDeviceFilterProxyModel();
    void testDeviceDetailsWidget();
    void testDevicePropertiesWidget();
};

void DiscoveryTest::testDeviceListModel()
{
    DeviceListModel model;
    QCOMPARE(model.rowCount(), 0);
    QVERIFY(!model.roleNames().isEmpty());

    DeviceInfo dev;
    dev.id = "test-1";
    dev.name = "Test Device";
    dev.vendor = "Qualcomm";
    dev.protocol = "Sahara";
    dev.transport = "USB";
    dev.status = "Connected";
    dev.bootMode = "EDL";
    dev.confidence = 85;
    dev.connected = true;

    model.addDevice(dev);
    QCOMPARE(model.rowCount(), 1);

    auto retrieved = model.deviceAt(0);
    QCOMPARE(retrieved.id, QString("test-1"));
    QCOMPARE(retrieved.vendor, QString("Qualcomm"));

    auto byId = model.deviceById("test-1");
    QCOMPARE(byId.name, QString("Test Device"));

    QVERIFY(!byId.name.isEmpty());

    DeviceInfo dev2;
    dev2.id = "test-2";
    dev2.name = "Device 2";
    model.addDevice(dev2);
    QCOMPARE(model.rowCount(), 2);

    dev2.name = "Updated Device 2";
    model.updateDevice(dev2);
    QCOMPARE(model.deviceById("test-2").name, QString("Updated Device 2"));

    model.removeDevice("test-1");
    QCOMPARE(model.rowCount(), 1);
    QVERIFY(model.deviceById("test-1").id.isEmpty());

    model.clear();
    QCOMPARE(model.rowCount(), 0);

    // Test data() with various roles
    model.addDevice(dev);
    auto idx = model.index(0, 0);
    QVERIFY(idx.isValid());
    QCOMPARE(model.data(idx, DeviceListModel::IdRole).toString(), QString("test-1"));
    QCOMPARE(model.data(idx, DeviceListModel::NameRole).toString(), QString("Test Device"));
    QCOMPARE(model.data(idx, DeviceListModel::VendorRole).toString(), QString("Qualcomm"));
    QCOMPARE(model.data(idx, DeviceListModel::ConnectedRole).toBool(), true);
    QCOMPARE(model.data(idx, DeviceListModel::ConfidenceRole).toInt(), 85);

    // Test setDevices
    QList<DeviceInfo> devices;
    DeviceInfo d1; d1.id = "d1"; d1.name = "D1";
    DeviceInfo d2; d2.id = "d2"; d2.name = "D2";
    devices << d1 << d2;
    model.setDevices(devices);
    QCOMPARE(model.count(), 2);
}

void DiscoveryTest::testDeviceTreeModel()
{
    DeviceTreeModel model;
    QCOMPARE(model.rowCount(), 0);
    QCOMPARE(model.columnCount(), 1);

    QList<DeviceInfo> devices;
    DeviceInfo d1; d1.id = "d1"; d1.name = "Dev1"; d1.vendor = "Qualcomm"; d1.protocol = "Sahara";
    DeviceInfo d2; d2.id = "d2"; d2.name = "Dev2"; d2.vendor = "Qualcomm"; d2.protocol = "Firehose";
    DeviceInfo d3; d3.id = "d3"; d3.name = "Dev3"; d3.vendor = "MediaTek"; d3.protocol = "DA";
    devices << d1 << d2 << d3;

    model.setDevices(devices);
    QVERIFY(model.rowCount() > 0);

    auto vendorIdx = model.index(0, 0);
    QVERIFY(vendorIdx.isValid());

    model.clear();
    QCOMPARE(model.rowCount(), 0);
}

void DiscoveryTest::testDeviceFilterProxyModel()
{
    DeviceListModel sourceModel;
    DeviceFilterProxyModel proxy;
    proxy.setSourceModel(&sourceModel);

    DeviceInfo d1; d1.id = "d1"; d1.name = "Sahara Device"; d1.vendor = "Qualcomm"; d1.protocol = "Sahara"; d1.connected = true;
    DeviceInfo d2; d2.id = "d2"; d2.name = "Firehose Device"; d2.vendor = "Qualcomm"; d2.protocol = "Firehose"; d2.connected = false;
    sourceModel.addDevice(d1);
    sourceModel.addDevice(d2);

    QCOMPARE(proxy.rowCount(), 2);

    proxy.setFilterText("Sahara");
    QCOMPARE(proxy.rowCount(), 1);

    proxy.setFilterText("");
    QCOMPARE(proxy.rowCount(), 2);

    proxy.setVendorFilter("Qualcomm");
    proxy.setProtocolFilter("");
    QCOMPARE(proxy.rowCount(), 2);

    proxy.setShowConnectedOnly(true);
    QCOMPARE(proxy.rowCount(), 1);

    proxy.setShowConnectedOnly(false);
    QCOMPARE(proxy.rowCount(), 2);

    QCOMPARE(proxy.filterText(), QString(""));
}

void DiscoveryTest::testDeviceDetailsWidget()
{
    DeviceDetailsWidget widget;
    QVERIFY(widget.width() > 0);

    DeviceInfo dev;
    dev.id = "test-1";
    dev.name = "Test Device";
    dev.vendor = "Qualcomm";
    widget.showDevice(dev);

    QSignalSpy connectSpy(&widget, &DeviceDetailsWidget::connectRequested);
    QSignalSpy disconnectSpy(&widget, &DeviceDetailsWidget::disconnectRequested);

    widget.clear();
}

void DiscoveryTest::testDevicePropertiesWidget()
{
    DevicePropertiesWidget widget;
    QVERIFY(widget.width() > 0);

    DeviceInfo dev;
    dev.id = "test-1";
    dev.name = "Test Device";
    widget.showProperties(dev);

    widget.clear();
}

QTEST_MAIN(DiscoveryTest)
#include "discovery_test.moc"
