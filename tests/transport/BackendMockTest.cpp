#include <catch2/catch_test_macros.hpp>
#include <mbootcore/transport/serial/ISerialBackend.hpp>
#include <mbootcore/transport/network/ITcpBackend.hpp>
#include <mbootcore/transport/network/IUdpBackend.hpp>
#include <mbootcore/transport/usb/UsbBackend.hpp>
#include "MockSerialBackend.hpp"
#include "MockTcpBackend.hpp"
#include "MockUdpBackend.hpp"
#include "MockUsbBackend.hpp"

using namespace mbootcore;
using namespace mbootcore::transport;
using namespace mbootcore::transport::serial;
using namespace mbootcore::transport::network;

TEST_CASE("BackendMockTest", "[backend][mock]") {

    // ===================== MockSerialBackend =====================

    SECTION("serial_defaultState_notOpen") {
        MockSerialBackend b;
        REQUIRE(!b.isOpen());
        REQUIRE(b.isAvailable());
    }

    SECTION("serial_openClose") {
        MockSerialBackend b;
        auto r = b.open("COM1", 115200, 8, 1, "none", "none", 65536);
        REQUIRE(r.isOk());
        REQUIRE(b.isOpen());
        REQUIRE(b.lastPortName() == "COM1");
        REQUIRE(b.lastBaudRate() == 115200);
        b.close();
        REQUIRE(!b.isOpen());
    }

    SECTION("serial_failOnOpen") {
        MockSerialBackend b;
        b.setFailOnOpen(true);
        auto r = b.open("/dev/ttyUSB0", 9600, 8, 1, "none", "none");
        REQUIRE(r.isError());
        REQUIRE(!b.isOpen());
    }

    SECTION("serial_writeRead") {
        MockSerialBackend b;
        REQUIRE(b.open("COM1", 115200, 8, 1, "none", "none").isOk());

        uint8_t wd[] = {0x01, 0x02, 0x03};
        auto wr = b.write(wd, sizeof(wd), std::chrono::seconds(1));
        REQUIRE(wr.isOk());
        REQUIRE(wr.value() == 3);
        REQUIRE(b.totalBytesWritten() == 3);
        REQUIRE(b.writes().size() == 1);
        REQUIRE(b.writes()[0] == ByteBuffer({0x01, 0x02, 0x03}));

        ByteBuffer rd = {0xAA, 0xBB};
        b.setReadData(rd);

        uint8_t buf[16] = {};
        auto rr = b.read(buf, sizeof(buf), std::chrono::seconds(1));
        REQUIRE(rr.isOk());
        REQUIRE(rr.value() == 2);
        REQUIRE(buf[0] == 0xAA);
        REQUIRE(buf[1] == 0xBB);
        REQUIRE(b.totalBytesRead() == 2);
    }

    SECTION("serial_failOnWriteRead") {
        MockSerialBackend b;
        REQUIRE(b.open("COM1", 115200, 8, 1, "none", "none").isOk());

        b.setFailOnWrite(true);
        uint8_t d1[] = {0x01};
        auto wr = b.write(d1, 1, std::chrono::seconds(1));
        REQUIRE(wr.isError());
        REQUIRE(wr.error() == ErrorCode::TransportWriteFailed);

        b.setFailOnRead(true);
        uint8_t buf[4] = {};
        auto rr = b.read(buf, sizeof(buf), std::chrono::seconds(1));
        REQUIRE(rr.isError());
        REQUIRE(rr.error() == ErrorCode::TransportReadFailed);
    }

    SECTION("serial_readTimeout") {
        MockSerialBackend b;
        REQUIRE(b.open("COM1", 115200, 8, 1, "none", "none").isOk());
        b.setReadTimeout(true);

        uint8_t buf[4] = {};
        auto rr = b.read(buf, sizeof(buf), std::chrono::seconds(1));
        REQUIRE(rr.isError());
        REQUIRE(rr.error() == ErrorCode::TransportTimeout);
    }

    SECTION("serial_cancel") {
        MockSerialBackend b;
        REQUIRE(b.open("COM1", 115200, 8, 1, "none", "none").isOk());
        REQUIRE(!b.wasCancelled());
        b.cancel();
        REQUIRE(b.wasCancelled());
    }

    SECTION("serial_flush") {
        MockSerialBackend b;
        REQUIRE(b.open("COM1", 115200, 8, 1, "none", "none").isOk());

        auto r = b.flush();
        REQUIRE(r.isOk());

        b.setFailOnFlush(true);
        r = b.flush();
        REQUIRE(r.isError());
    }

    SECTION("serial_callRecords") {
        MockSerialBackend b;
        REQUIRE(b.open("COM1", 115200, 8, 1, "none", "none").isOk());
        uint8_t d[] = {0x01};
        REQUIRE(b.write(d, 1, std::chrono::seconds(1)).isOk());
        b.close();

        auto recs = b.records();
        REQUIRE(recs.size() == 3);
        REQUIRE(recs[0].type == MockSerialBackend::CallRecord::Open);
        REQUIRE(recs[1].type == MockSerialBackend::CallRecord::Write);
        REQUIRE(recs[1].size == 1);
        REQUIRE(recs[2].type == MockSerialBackend::CallRecord::Close);
    }

    // ===================== MockTcpBackend =====================

    SECTION("tcp_defaultState_notConnected") {
        MockTcpBackend b;
        REQUIRE(!b.isConnected());
        REQUIRE(b.isAvailable());
    }

    SECTION("tcp_openClose") {
        MockTcpBackend b;
        auto r = b.open("192.168.1.1", 9008, true, std::chrono::seconds(5));
        REQUIRE(r.isOk());
        REQUIRE(b.isConnected());
        REQUIRE(b.lastHost() == "192.168.1.1");
        REQUIRE(b.lastPort() == 9008);
        b.close();
        REQUIRE(!b.isConnected());
    }

    SECTION("tcp_failOnOpen") {
        MockTcpBackend b;
        b.setFailOnOpen(true);
        auto r = b.open("10.0.0.1", 80, false, std::chrono::seconds(3));
        REQUIRE(r.isError());
        REQUIRE(!b.isConnected());
    }

    SECTION("tcp_writeRead") {
        MockTcpBackend b;
        REQUIRE(b.open("127.0.0.1", 8080, false, std::chrono::seconds(5)).isOk());

        uint8_t wd[] = {0x10, 0x20, 0x30, 0x40};
        auto wr = b.write(wd, sizeof(wd), std::chrono::seconds(1));
        REQUIRE(wr.isOk());
        REQUIRE(wr.value() == 4);
        REQUIRE(b.totalBytesWritten() == 4);
        REQUIRE(b.writes()[0] == ByteBuffer({0x10, 0x20, 0x30, 0x40}));

        ByteBuffer rd = {0xFF, 0xFE};
        b.setReadData(rd);

        uint8_t buf[16] = {};
        auto rr = b.read(buf, sizeof(buf), std::chrono::seconds(1));
        REQUIRE(rr.isOk());
        REQUIRE(rr.value() == 2);
        REQUIRE(buf[0] == 0xFF);
    }

    SECTION("tcp_readTimeout") {
        MockTcpBackend b;
        REQUIRE(b.open("127.0.0.1", 8080, false, std::chrono::seconds(5)).isOk());
        b.setReadTimeout(true);

        uint8_t buf[4] = {};
        auto rr = b.read(buf, sizeof(buf), std::chrono::seconds(1));
        REQUIRE(rr.isError());
        REQUIRE(rr.error() == ErrorCode::TransportTimeout);
    }

    SECTION("tcp_disconnectOnEmptyRead") {
        MockTcpBackend b;
        REQUIRE(b.open("127.0.0.1", 8080, false, std::chrono::seconds(5)).isOk());

        b.setDisconnectOnEmptyRead(true);
        uint8_t buf[4] = {};
        auto rr = b.read(buf, sizeof(buf), std::chrono::seconds(1));
        REQUIRE(rr.isError());
        REQUIRE(!b.isConnected());
    }

    SECTION("tcp_callRecords") {
        MockTcpBackend b;
        REQUIRE(b.open("host", 1234, false, std::chrono::seconds(3)).isOk());
        uint8_t d[] = {0x01, 0x02};
        REQUIRE(b.write(d, 2, std::chrono::seconds(1)).isOk());
        b.close();

        auto recs = b.records();
        REQUIRE(recs.size() == 3);
        REQUIRE(recs[0].type == MockTcpBackend::CallRecord::Open);
        REQUIRE(recs[1].type == MockTcpBackend::CallRecord::Write);
        REQUIRE(recs[1].size == 2);
        REQUIRE(recs[2].type == MockTcpBackend::CallRecord::Close);
    }

    // ===================== MockUdpBackend =====================

    SECTION("udp_defaultState_notConnected") {
        MockUdpBackend b;
        REQUIRE(!b.isConnected());
        REQUIRE(b.isAvailable());
    }

    SECTION("udp_openClose") {
        MockUdpBackend b;
        auto r = b.open("0.0.0.0", 0, "127.0.0.1", 9999,
                         std::chrono::seconds(5), false);
        REQUIRE(r.isOk());
        REQUIRE(b.isConnected());
        REQUIRE(b.lastLocalAddr() == "0.0.0.0");
        REQUIRE(b.lastRemotePort() == 9999);
        REQUIRE(b.lastBroadcast() == false);
        b.close();
        REQUIRE(!b.isConnected());
    }

    SECTION("udp_broadcastFlag") {
        MockUdpBackend b;
        auto r = b.open("0.0.0.0", 0, "255.255.255.255", 8888,
                         std::chrono::seconds(5), true);
        REQUIRE(r.isOk());
        REQUIRE(b.lastBroadcast());
    }

    SECTION("udp_sendRecv") {
        MockUdpBackend b;
        REQUIRE(b.open("0.0.0.0", 0, "127.0.0.1", 9999, std::chrono::seconds(5)).isOk());

        uint8_t sd[] = {0xDE, 0xAD, 0xBE, 0xEF};
        auto sr = b.send(sd, sizeof(sd), std::chrono::seconds(1));
        REQUIRE(sr.isOk());
        REQUIRE(sr.value() == 4);
        REQUIRE(b.totalBytesSent() == 4);
        REQUIRE(b.sends()[0] == ByteBuffer({0xDE, 0xAD, 0xBE, 0xEF}));

        ByteBuffer rd = {0xCA, 0xFE};
        b.setRecvData(rd);

        uint8_t buf[16] = {};
        auto rr = b.recv(buf, sizeof(buf), std::chrono::seconds(1));
        REQUIRE(rr.isOk());
        REQUIRE(rr.value() == 2);
    }

    SECTION("udp_recvTimeout") {
        MockUdpBackend b;
        REQUIRE(b.open("0.0.0.0", 0, "127.0.0.1", 9999, std::chrono::seconds(5)).isOk());
        b.setRecvTimeout(true);

        uint8_t buf[4] = {};
        auto rr = b.recv(buf, sizeof(buf), std::chrono::seconds(1));
        REQUIRE(rr.isError());
        REQUIRE(rr.error() == ErrorCode::TransportTimeout);
    }

    SECTION("udp_callRecords") {
        MockUdpBackend b;
        REQUIRE(b.open("0.0.0.0", 0, "10.0.0.1", 1234, std::chrono::seconds(3)).isOk());
        uint8_t d[] = {0x01};
        REQUIRE(b.send(d, 1, std::chrono::seconds(1)).isOk());
        b.close();

        auto recs = b.records();
        REQUIRE(recs.size() == 3);
        REQUIRE(recs[0].type == MockUdpBackend::CallRecord::Open);
        REQUIRE(recs[1].type == MockUdpBackend::CallRecord::Send);
        REQUIRE(recs[2].type == MockUdpBackend::CallRecord::Close);
    }

    // ===================== MockUsbBackend =====================

    SECTION("usb_defaultState_notOpen") {
        MockUsbBackend b;
        REQUIRE(!b.isOpen());
        REQUIRE(b.isAvailable());
    }

    SECTION("usb_openClose") {
        MockUsbBackend b;
        auto r = b.open(0x1234, 0x5678, 0);
        REQUIRE(r.isOk());
        REQUIRE(b.isOpen());
        b.close();
        REQUIRE(!b.isOpen());
    }

    SECTION("usb_bulkWriteRead") {
        MockUsbBackend b;
        REQUIRE(b.open(0x1234, 0x5678, 0).isOk());

        uint8_t wd[] = {0x01, 0x02, 0x03};
        auto wr = b.bulkWrite(0x01, wd, sizeof(wd), std::chrono::seconds(1));
        REQUIRE(wr.isOk());
        REQUIRE(wr.value() == 3);
        REQUIRE(b.totalBytesWritten() == 3);

        ByteBuffer rd = {0xAA, 0xBB};
        b.setReadData(rd);
        uint8_t buf[16] = {};
        auto rr = b.bulkRead(0x81, buf, sizeof(buf), std::chrono::seconds(1));
        REQUIRE(rr.isOk());
        REQUIRE(rr.value() == 2);
        REQUIRE(buf[0] == 0xAA);
    }

    SECTION("usb_callRecords") {
        MockUsbBackend b;
        REQUIRE(b.open(0x1234, 0x5678, 0).isOk());
        uint8_t d[] = {0x01};
        REQUIRE(b.bulkWrite(0x01, d, sizeof(d), std::chrono::seconds(1)).isOk());
        b.close();

        auto recs = b.records();
        REQUIRE(recs.size() == 3);
        REQUIRE(recs[0].type == MockUsbBackend::CallRecord::Open);
        REQUIRE(recs[1].type == MockUsbBackend::CallRecord::BulkWrite);
        REQUIRE(recs[2].type == MockUsbBackend::CallRecord::Close);
    }
}
