#include <QTest>
#include <QApplication>
#include <QSignalSpy>
#include "gui/vendor/VendorManagerWidget.hpp"

class VendorTest : public QObject {
    Q_OBJECT
private slots:
    void testVendorManagerWidget();
};

void VendorTest::testVendorManagerWidget()
{
    VendorManagerWidget w;
    QVERIFY(w.width() > 0);
    w.loadVendors();
}

QTEST_MAIN(VendorTest)
#include "vendor_test.moc"
