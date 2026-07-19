#include <QTest>
#include <QApplication>
#include "gui/transport/TransportMonitorWidget.hpp"

class TransportTest : public QObject {
    Q_OBJECT
private slots:
    void testTransportMonitorWidget();
};

void TransportTest::testTransportMonitorWidget()
{
    TransportMonitorWidget w;
    QVERIFY(w.width() > 0);

    QVariantMap stats;
    stats["latency"] = 5.0;
    stats["bandwidth"] = 480.0;
    stats["packets"] = 1000;

    w.setUsbStats(stats);
    w.setSerialStats(stats);
    w.setTcpStats(stats);
    w.resetStats();
}

QTEST_MAIN(TransportTest)
#include "transport_test.moc"
