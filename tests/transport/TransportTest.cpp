#include <catch2/catch_test_macros.hpp>
#include "MockTransport.hpp"
#include "MockUsbBackend.hpp"
#include <mbootcore/transport/VirtualTransports.hpp>
#include <mbootcore/transport/TransportManager.hpp>
#include <mbootcore/transport/TransportFactory.hpp>
#include <mbootcore/transport/UsbTransport.hpp>
#include <mbootcore/transport/SerialTransport.hpp>
#include <mbootcore/transport/TcpTransport.hpp>
#include <mbootcore/transport/UdpTransport.hpp>
#include <mbootcore/transport/SerialEnumerator.hpp>

using namespace mbootcore;
using namespace mbootcore::transport;

TEST_CASE("VirtualUsbTransport", "[transport]") {
    SECTION("defaultConstruction_notOpen") {
        VirtualUsbTransport vt;
        REQUIRE(!vt.isOpen());
        REQUIRE(vt.transportType() == TransportType::Virtual);
        REQUIRE(vt.state() == TransportState::Closed);
    }

    SECTION("openClose") {
        VirtualUsbTransport vt;
        auto r = vt.open();
        REQUIRE(r.isOk());
        REQUIRE(vt.isOpen());
        vt.close();
        REQUIRE(!vt.isOpen());
        REQUIRE(vt.state() == TransportState::Closed);
    }

    SECTION("openFail") {
        VirtualTransportConfig cfg;
        cfg.failOnOpen = true;
        VirtualUsbTransport vt(cfg);
        auto r = vt.open();
        REQUIRE(r.isError());
        REQUIRE(!vt.isOpen());
    }

    SECTION("writeRead") {
        VirtualUsbTransport vt;
        REQUIRE(vt.open().isOk());

        ByteBuffer data{0x01, 0x02, 0x03, 0x04};
        auto wr = vt.write(data, std::chrono::seconds(1));
        REQUIRE(wr.isOk());
        REQUIRE(wr.value() == data.size());

        vt.injectData(data);
        ByteBuffer buf;
        auto rr = vt.read(buf, 1, 1024, std::chrono::seconds(1));
        REQUIRE(rr.isOk());
        REQUIRE(rr.value() == data.size());
        REQUIRE(buf == data);
    }

    SECTION("injectReadMultiple") {
        VirtualUsbTransport vt;
        REQUIRE(vt.open().isOk());

        vt.injectData({0x01, 0x02});
        vt.injectData({0x03, 0x04, 0x05});

        ByteBuffer buf1;
        auto r1 = vt.read(buf1, 1, 1024, std::chrono::seconds(1));
        REQUIRE(r1.value() == 2);

        ByteBuffer buf2;
        auto r2 = vt.read(buf2, 1, 1024, std::chrono::seconds(1));
        REQUIRE(r2.value() == 3);
    }

    SECTION("readTimeout") {
        VirtualUsbTransport vt;
        REQUIRE(vt.open().isOk());

        ByteBuffer buf;
        auto r = vt.read(buf, 1, 1024, std::chrono::milliseconds(10));
        REQUIRE(r.isError());
    }

    SECTION("sendZLP") {
        VirtualUsbTransport vt;
        REQUIRE(vt.open().isOk());
        auto r = vt.sendZLP(std::chrono::seconds(1));
        REQUIRE(r.isOk());
    }

    SECTION("sendZLP_whenClosed") {
        VirtualUsbTransport vt;
        auto r = vt.sendZLP(std::chrono::seconds(1));
        REQUIRE(r.isError());
    }

    SECTION("cancel") {
        VirtualUsbTransport vt;
        REQUIRE(vt.open().isOk());
        vt.cancel();
        ByteBuffer data{0x01};
        auto wr = vt.write(data, std::chrono::seconds(1));
        REQUIRE(wr.isError());
    }

    SECTION("failOnWrite") {
        VirtualTransportConfig cfg;
        cfg.failOnWrite = true;
        VirtualUsbTransport vt(cfg);
        REQUIRE(vt.open().isOk());
        auto r = vt.write({0x01}, std::chrono::seconds(1));
        REQUIRE(r.isError());
    }

    SECTION("failOnRead") {
        VirtualTransportConfig cfg;
        cfg.failOnRead = true;
        VirtualUsbTransport vt(cfg);
        REQUIRE(vt.open().isOk());
        vt.injectData({0x01});
        ByteBuffer buf;
        auto r = vt.read(buf, 1, 1024, std::chrono::seconds(1));
        REQUIRE(r.isError());
    }

    SECTION("disconnect") {
        VirtualUsbTransport vt;
        REQUIRE(vt.open().isOk());
        vt.triggerDisconnect();
        REQUIRE(!vt.isOpen());
        REQUIRE(vt.state() == TransportState::Error);

        vt.triggerReconnect();
        REQUIRE(vt.isOpen());
    }

    SECTION("flushReset") {
        VirtualUsbTransport vt;
        REQUIRE(vt.open().isOk());
        vt.injectData({0x01, 0x02});
        auto r = vt.flush();
        REQUIRE(r.isOk());

        r = vt.reset();
        REQUIRE(r.isOk());
    }

    SECTION("reconnect") {
        VirtualUsbTransport vt;
        REQUIRE(vt.open().isOk());
        auto r = vt.reconnect();
        REQUIRE(r.isOk());
        REQUIRE(vt.isOpen());
    }

    SECTION("statistics") {
        VirtualUsbTransport vt;
        REQUIRE(vt.open().isOk());

        REQUIRE(vt.write({0x01, 0x02}, std::chrono::seconds(1)).isOk());
        vt.injectData({0x03, 0x04});
        ByteBuffer buf;
        REQUIRE(vt.read(buf, 1, 1024, std::chrono::seconds(1)).isOk());

        auto s = vt.statistics();
        REQUIRE(s.bytesWritten == 2);
        REQUIRE(s.bytesRead == 2);
        REQUIRE(s.writeOperations == 1);
        REQUIRE(s.readOperations == 1);
    }

    SECTION("config") {
        VirtualUsbTransport vt;
        auto& cfg = vt.virtConfig();
        cfg.simulateLatency = true;
        cfg.minLatency = std::chrono::milliseconds(5);
        REQUIRE(vt.virtConfig().simulateLatency);
        REQUIRE(vt.virtConfig().minLatency.count() == 5);
    }

    SECTION("faultConfig") {
        VirtualUsbTransport vt;
        auto& fc = vt.faultConfig();
        fc.enableReadTimeout = true;
        fc.enableWriteTimeout = true;
        REQUIRE(vt.faultConfig().enableReadTimeout);
    }

    SECTION("totalBytesTracked") {
        VirtualUsbTransport vt;
        REQUIRE(vt.open().isOk());
        REQUIRE(vt.write({0x01, 0x02, 0x03}, std::chrono::seconds(1)).isOk());
        vt.injectData({0x04, 0x05});
        ByteBuffer buf;
        REQUIRE(vt.read(buf, 1, 1024, std::chrono::seconds(1)).isOk());
        REQUIRE(vt.totalBytesWritten() == 3);
        REQUIRE(vt.totalBytesRead() == 2);
        REQUIRE(vt.totalBytesInjected() == 2);
    }

    SECTION("endpoint") {
        VirtualUsbTransport vt;
        auto ep = vt.endpoint();
        REQUIRE(ep.type == TransportType::Virtual);
        REQUIRE(!ep.description.empty());
    }

    SECTION("name") {
        VirtualUsbTransport vt;
        REQUIRE(vt.name() == "VirtualUsbTransport");
    }
}

TEST_CASE("TransportManager", "[transport]") {
    SECTION("default") {
        TransportManager mgr;
        REQUIRE(mgr.count() == 0);
        REQUIRE(mgr.openCount() == 0);
    }

    SECTION("addGet") {
        TransportManager mgr;
        auto t = std::make_unique<MockTransport>();
        auto* ptr = t.get();
        REQUIRE(mgr.add("test", std::move(t)));
        REQUIRE(mgr.get("test") == ptr);
        REQUIRE(mgr.count() == 1);
    }

    SECTION("addDuplicate") {
        TransportManager mgr;
        REQUIRE(mgr.add("a", std::make_unique<MockTransport>()));
        REQUIRE(!mgr.add("a", std::make_unique<MockTransport>()));
    }

    SECTION("addNull") {
        TransportManager mgr;
        REQUIRE(!mgr.add("null", nullptr));
    }

    SECTION("getNonExistent") {
        TransportManager mgr;
        REQUIRE(mgr.get("nonexistent") == nullptr);
    }

    SECTION("remove") {
        TransportManager mgr;
        REQUIRE(mgr.add("a", std::make_unique<MockTransport>()));
        REQUIRE(mgr.remove("a"));
        REQUIRE(mgr.get("a") == nullptr);
        REQUIRE(mgr.count() == 0);
    }

    SECTION("removeNonExistent") {
        TransportManager mgr;
        REQUIRE(!mgr.remove("nonexistent"));
    }

    SECTION("openClose") {
        TransportManager mgr;
        REQUIRE(mgr.add("a", std::make_unique<MockTransport>()));
        REQUIRE(mgr.open("a"));
        REQUIRE(mgr.openCount() == 1);
        REQUIRE(mgr.close("a"));
        REQUIRE(mgr.openCount() == 0);
    }

    SECTION("openNonExistent") {
        TransportManager mgr;
        REQUIRE(!mgr.open("nope"));
        REQUIRE(!mgr.close("nope"));
    }

    SECTION("closeAll") {
        TransportManager mgr;
        REQUIRE(mgr.add("a", std::make_unique<MockTransport>()));
        REQUIRE(mgr.add("b", std::make_unique<MockTransport>()));
        mgr.open("a");
        mgr.open("b");
        REQUIRE(mgr.openCount() == 2);
        mgr.closeAll();
        REQUIRE(mgr.openCount() == 0);
    }

    SECTION("reconnect") {
        TransportManager mgr;
        REQUIRE(mgr.add("a", std::make_unique<MockTransport>()));
        mgr.open("a");
        REQUIRE(mgr.reconnect("a"));
    }

    SECTION("ids") {
        TransportManager mgr;
        REQUIRE(mgr.add("a", std::make_unique<MockTransport>()));
        REQUIRE(mgr.add("b", std::make_unique<MockTransport>()));
        auto ids = mgr.ids();
        REQUIRE(ids.size() == 2);
    }

    SECTION("openIds") {
        TransportManager mgr;
        REQUIRE(mgr.add("a", std::make_unique<MockTransport>()));
        REQUIRE(mgr.add("b", std::make_unique<MockTransport>()));
        mgr.open("a");
        auto oids = mgr.openIds();
        REQUIRE(oids.size() == 1);
        REQUIRE(oids[0] == "a");
    }

    SECTION("callback") {
        TransportManager mgr;
        REQUIRE(mgr.add("a", std::make_unique<MockTransport>()));
        std::string lastId;
        TransportState lastState{TransportState::Closed};
        mgr.setCallback([&](const std::string& id, TransportState s) {
            lastId = id; lastState = s;
        });
        mgr.open("a");
        REQUIRE(lastId == "a");
        REQUIRE(lastState == TransportState::Open);
        mgr.close("a");
        REQUIRE(lastId == "a");
        REQUIRE(lastState == TransportState::Closed);
    }

    SECTION("totalStatistics") {
        TransportManager mgr;
        auto t = std::make_unique<MockTransport>();
        t->setReadResult(Result<size_t>(0));
        t->setWriteResult(Result<size_t>(1));
        REQUIRE(mgr.add("a", std::move(t)));
        mgr.open("a");
        auto* tp = mgr.get("a");
        (void)tp->write({0x01}, std::chrono::seconds(1));
        REQUIRE(true);
    }

    SECTION("resetStatistics") {
        TransportManager mgr;
        auto virt = std::make_unique<VirtualUsbTransport>();
        REQUIRE(virt->open().isOk());
        auto* vp = virt.get();
        REQUIRE(mgr.add("v", std::move(virt)));

        // Accumulate statistics through the transport
        (void)vp->write({0x01, 0x02}, std::chrono::seconds(1));
        vp->injectData({0x03, 0x04});
        ByteBuffer buf;
        (void)vp->read(buf, 1, 1024, std::chrono::seconds(1));

        auto before = mgr.totalStatistics();
        REQUIRE(before.bytesWritten > 0);

        // resetStatistics must NOT clear the transport's internal counters
        mgr.resetStatistics("v");
        auto after = mgr.totalStatistics();
        REQUIRE(after.bytesWritten == before.bytesWritten);
        REQUIRE(after.bytesRead == before.bytesRead);
    }

    SECTION("resetAllStatistics") {
        TransportManager mgr;
        auto virt1 = std::make_unique<VirtualUsbTransport>();
        REQUIRE(virt1->open().isOk());
        auto* vp1 = virt1.get();
        REQUIRE(mgr.add("v1", std::move(virt1)));

        auto virt2 = std::make_unique<VirtualUsbTransport>();
        REQUIRE(virt2->open().isOk());
        auto* vp2 = virt2.get();
        REQUIRE(mgr.add("v2", std::move(virt2)));

        (void)vp1->write({0x01}, std::chrono::seconds(1));
        (void)vp2->write({0x02, 0x03}, std::chrono::seconds(1));

        auto before = mgr.totalStatistics();
        REQUIRE(before.bytesWritten == 3);

        mgr.resetAllStatistics();
        auto after = mgr.totalStatistics();
        // resetAllStatistics must NOT clear the transport's internal counters
        REQUIRE(after.bytesWritten == before.bytesWritten);
    }

    SECTION("getOrCreate") {
        TransportManager mgr;
        auto* t = mgr.getOrCreate("new", TransportType::Mock);
        REQUIRE(t != nullptr);
        REQUIRE(mgr.count() == 1);
        auto* t2 = mgr.getOrCreate("new", TransportType::USB);
        REQUIRE(t == t2);
    }
}

TEST_CASE("TransportFactory", "[transport]") {
    SECTION("createMock") {
        auto t = TransportFactory::createMock();
        REQUIRE(t != nullptr);
    }

    SECTION("createVirtual") {
        auto t = TransportFactory::createVirtual();
        REQUIRE(t != nullptr);
    }

    SECTION("createMockOpenable") {
        auto t = TransportFactory::createMock();
        auto r = t->open();
        REQUIRE(r.isOk());
        REQUIRE(t->isOpen());
    }
}

TEST_CASE("UsbTransport", "[transport]") {
    SECTION("construction_defaultState") {
        UsbTransport ut(0x05C6, 0x9008);
        REQUIRE(!ut.isOpen());
        REQUIRE(ut.transportType() == TransportType::USB);
        REQUIRE(ut.name() == "UsbTransport");
    }

    SECTION("openClose") {
        UsbTransport ut(0x05C6, 0x9008);
        auto backend = std::make_unique<MockUsbBackend>();
        auto* bp = backend.get();
        ut.setBackend(std::move(backend));

        auto r = ut.open();
        REQUIRE(r.isOk());
        REQUIRE(ut.isOpen());

        ut.close();
        REQUIRE(!ut.isOpen());
        REQUIRE(bp->records().size() >= 2);
    }

    SECTION("openFail") {
        UsbTransport ut(0x05C6, 0x9008);
        auto backend = std::make_unique<MockUsbBackend>();
        backend->setFailOnOpen(true);
        ut.setBackend(std::move(backend));

        auto r = ut.open();
        REQUIRE(r.isError());
        REQUIRE(!ut.isOpen());
    }

    SECTION("write") {
        UsbTransport ut(0x05C6, 0x9008);
        auto backend = std::make_unique<MockUsbBackend>();
        ut.setBackend(std::move(backend));
        REQUIRE(ut.open().isOk());

        ByteBuffer data{0x10, 0x20, 0x30};
        auto r = ut.write(data, std::chrono::seconds(1));
        REQUIRE(r.isOk());
        REQUIRE(r.value() == data.size());
    }

    SECTION("writeFail") {
        UsbTransport ut(0x05C6, 0x9008);
        auto backend = std::make_unique<MockUsbBackend>();
        backend->setFailOnWrite(true);
        ut.setBackend(std::move(backend));
        REQUIRE(ut.open().isOk());

        auto r = ut.write({0x01}, std::chrono::seconds(1));
        REQUIRE(r.isError());
    }

    SECTION("read") {
        UsbTransport ut(0x05C6, 0x9008);
        auto backend = std::make_unique<MockUsbBackend>();
        backend->setReadData({0xAA, 0xBB});
        ut.setBackend(std::move(backend));
        REQUIRE(ut.open().isOk());

        ByteBuffer buf;
        auto r = ut.read(buf, 1, 1024, std::chrono::seconds(1));
        REQUIRE(r.isOk());
        REQUIRE(r.value() == 2);
        REQUIRE(buf.size() == 2);
    }

    SECTION("readFail") {
        UsbTransport ut(0x05C6, 0x9008);
        auto backend = std::make_unique<MockUsbBackend>();
        backend->setFailOnRead(true);
        ut.setBackend(std::move(backend));
        REQUIRE(ut.open().isOk());

        ByteBuffer buf;
        auto r = ut.read(buf, 1, 1024, std::chrono::seconds(1));
        REQUIRE(r.isError());
    }

    SECTION("cancel") {
        UsbTransport ut(0x05C6, 0x9008);
        auto backend = std::make_unique<MockUsbBackend>();
        ut.setBackend(std::move(backend));
        REQUIRE(ut.open().isOk());

        ut.cancel();
        auto r = ut.write({0x01}, std::chrono::seconds(1));
        REQUIRE(r.isError());
    }

    SECTION("sendZLP") {
        UsbTransport ut(0x05C6, 0x9008);
        auto backend = std::make_unique<MockUsbBackend>();
        ut.setBackend(std::move(backend));
        REQUIRE(ut.open().isOk());

        auto r = ut.sendZLP(std::chrono::seconds(1));
        REQUIRE(r.isOk());
    }

    SECTION("flush") {
        UsbTransport ut(0x05C6, 0x9008);
        auto backend = std::make_unique<MockUsbBackend>();
        ut.setBackend(std::move(backend));
        REQUIRE(ut.open().isOk());

        auto r = ut.flush();
        REQUIRE(r.isOk());
    }

    SECTION("reset") {
        UsbTransport ut(0x05C6, 0x9008);
        auto backend = std::make_unique<MockUsbBackend>();
        ut.setBackend(std::move(backend));
        REQUIRE(ut.open().isOk());

        auto r = ut.reset();
        REQUIRE(r.isOk());
    }

    SECTION("reconnect") {
        UsbTransport ut(0x05C6, 0x9008);
        auto backend = std::make_unique<MockUsbBackend>();
        ut.setBackend(std::move(backend));
        REQUIRE(ut.open().isOk());

        auto r = ut.reconnect();
        REQUIRE(r.isOk());
    }

    SECTION("statistics") {
        UsbTransport ut(0x05C6, 0x9008);
        auto backend = std::make_unique<MockUsbBackend>();
        ut.setBackend(std::move(backend));
        REQUIRE(ut.open().isOk());

        REQUIRE(ut.write({0x01}, std::chrono::seconds(1)).isOk());
        REQUIRE(ut.write({0x02, 0x03}, std::chrono::seconds(1)).isOk());
        auto s = ut.statistics();
        REQUIRE(s.bytesWritten == 3);
        REQUIRE(s.writeOperations == 2);
    }

    SECTION("endpoint") {
        UsbTransport ut(0x05C6, 0x9008);
        auto ep = ut.endpoint();
        REQUIRE(ep.type == TransportType::USB);
        REQUIRE(ep.vendorId == 0x05C6);
        REQUIRE(ep.productId == 0x9008);
    }

    SECTION("config") {
        UsbTransport ut(0x05C6, 0x9008);
        TransportConfig cfg;
        cfg.timeout = std::chrono::seconds(30);
        ut.setConfig(cfg);
        REQUIRE(ut.config().timeout.count() == 30000);
    }
}

TEST_CASE("SerialTransport", "[transport]") {
    SECTION("construction") {
        SerialTransport st("COM1", 115200);
        REQUIRE(!st.isOpen());
        REQUIRE(st.transportType() == TransportType::Serial);
        REQUIRE(st.name() == "SerialTransport");
    }

    SECTION("endpoint") {
        SerialTransport st("COM42");
        auto ep = st.endpoint();
        REQUIRE(ep.type == TransportType::Serial);
        REQUIRE(ep.address == "COM42");
    }
}

TEST_CASE("TcpTransport", "[transport]") {
    SECTION("construction") {
        TcpTransport tt("127.0.0.1", 1234);
        REQUIRE(!tt.isOpen());
        REQUIRE(tt.transportType() == TransportType::TCP);
        REQUIRE(tt.name() == "TcpTransport");
    }

    SECTION("endpoint") {
        TcpTransport tt("10.0.0.1", 8080);
        auto ep = tt.endpoint();
        REQUIRE(ep.type == TransportType::TCP);
        REQUIRE(ep.address == "10.0.0.1");
        REQUIRE(ep.port == 8080);
    }
}

TEST_CASE("UdpTransport", "[transport]") {
    SECTION("construction") {
        UdpTransport ut;
        REQUIRE(!ut.isOpen());
        REQUIRE(ut.transportType() == TransportType::UDP);
        REQUIRE(ut.name() == "UdpTransport");
    }

    SECTION("openClose") {
        UdpTransport ut("0.0.0.0", 0, "127.0.0.1", 9999);
        auto r = ut.open();
        REQUIRE(r.isOk());
        REQUIRE(ut.isOpen());
        ut.close();
        REQUIRE(!ut.isOpen());
    }

    SECTION("write") {
        UdpTransport ut("0.0.0.0", 0, "127.0.0.1", 9999);
        REQUIRE(ut.open().isOk());
        auto r = ut.write({0x01, 0x02}, std::chrono::seconds(1));
        REQUIRE(r.isOk());
    }

    SECTION("sendZLP") {
        UdpTransport ut("0.0.0.0", 0, "127.0.0.1", 9999);
        REQUIRE(ut.open().isOk());
        auto r = ut.sendZLP(std::chrono::seconds(1));
        REQUIRE(r.isOk());
    }

    SECTION("endpoint") {
        UdpTransport ut("0.0.0.0", 0, "192.168.1.100", 9999);
        auto ep = ut.endpoint();
        REQUIRE(ep.type == TransportType::UDP);
        REQUIRE(ep.address == "192.168.1.100");
        REQUIRE(ep.port == 9999);
    }
}

TEST_CASE("SerialEnumerator", "[transport]") {
    SECTION("portCount") {
        auto count = SerialEnumerator::portCount();
        REQUIRE(count <= 100);
    }

    SECTION("enumerate") {
        auto ports = SerialEnumerator::enumerate();
        REQUIRE(ports.size() == SerialEnumerator::portCount());
    }

    SECTION("findFirst_noCrash") {
        auto entry = SerialEnumerator::findFirst();
        (void)entry;
    }

    SECTION("findByDescription_noCrash") {
        auto ports = SerialEnumerator::findByDescription("USB");
        (void)ports;
    }
}
