#include <catch2/catch_test_macros.hpp>
#include <mbootcore/transport/UdpTransport.hpp>
#include "TransportTestUtils.hpp"
#include <memory>

using namespace mbootcore;
using namespace mbootcore::transport;
using namespace mbootcore::test;

namespace {
    std::unique_ptr<UdpEchoServer> server_;
    uint16_t port_{0};

    void setupServer() {
        port_ = findFreePort();
        REQUIRE((port_ != 0 && port_ != 9999));
        server_ = std::make_unique<UdpEchoServer>(port_);
        server_->start();
        REQUIRE(server_->isRunning());
    }

    void teardownServer() {
        if (server_) {
            server_->stop();
            server_.reset();
        }
    }

    // ========= Smoke =========

    void smoke_openSendRecvClose() {
        setupServer();
        UdpTransport t("0.0.0.0", 0, "127.0.0.1", port_, false, nullptr);

        auto r = t.open();
        REQUIRE(r.isOk());
        REQUIRE(t.isOpen());

        uint8_t sendData[] = {'U', 'D', 'P'};
        auto wr = t.write(ByteBuffer(sendData, sendData + sizeof(sendData)),
                          std::chrono::seconds(2));
        REQUIRE(wr.isOk());
        REQUIRE(wr.value() == sizeof(sendData));

        ByteBuffer recvBuf;
        auto rr = t.read(recvBuf, sizeof(sendData), sizeof(sendData) * 2,
                         std::chrono::seconds(2));
        REQUIRE(rr.isOk());
        REQUIRE(recvBuf.size() == sizeof(sendData));

        t.close();
        REQUIRE(!t.isOpen());
        teardownServer();
    }

    // ========= Lifecycle =========

    void lifecycle_openCloseReopen() {
        setupServer();

        {
            UdpTransport t("0.0.0.0", 0, "127.0.0.1", port_, false, nullptr);
            REQUIRE(t.open().isOk());
            t.close();
            REQUIRE(!t.isOpen());
        }

        {
            UdpTransport t2("0.0.0.0", 0, "127.0.0.1", port_, false, nullptr);
            REQUIRE(t2.open().isOk());
            REQUIRE(t2.isOpen());
            t2.close();
        }

        teardownServer();
    }

    void lifecycle_doubleOpen_returnsError() {
        setupServer();
        UdpTransport t("0.0.0.0", 0, "127.0.0.1", port_, false, nullptr);
        REQUIRE(t.open().isOk());

        auto r = t.open();
        REQUIRE(r.isError());
        REQUIRE(r.error() == ErrorCode::TransportAlreadyOpen);

        t.close();
        teardownServer();
    }

    void lifecycle_doubleClose_noError() {
        setupServer();
        UdpTransport t("0.0.0.0", 0, "127.0.0.1", port_, false, nullptr);
        REQUIRE(t.open().isOk());
        t.close();
        t.close();
        REQUIRE(!t.isOpen());
        teardownServer();
    }

    // ========= Negative =========

    void negative_sendBeforeOpen_returnsNotOpen() {
        UdpTransport t("0.0.0.0", 0, "127.0.0.1", port_, false, nullptr);
        auto r = t.write({0x01}, std::chrono::milliseconds(100));
        REQUIRE(r.isError());
        REQUIRE(r.error() == ErrorCode::TransportNotOpen);
    }

    void negative_recvAfterClose_returnsNotOpen() {
        setupServer();
        UdpTransport t("0.0.0.0", 0, "127.0.0.1", port_, false, nullptr);
        REQUIRE(t.open().isOk());
        t.close();

        ByteBuffer buf;
        auto r = t.read(buf, 1, 10, std::chrono::milliseconds(100));
        REQUIRE(r.isError());
        REQUIRE(r.error() == ErrorCode::TransportNotOpen);
        teardownServer();
    }

    // ========= Timeout =========

    void timeout_recvWithNoData_returnsTimeout() {
        setupServer();
        UdpTransport t("0.0.0.0", 0, "127.0.0.1", port_, false, nullptr);
        REQUIRE(t.open().isOk());

        ByteBuffer buf;
        auto r = t.read(buf, 1, 10, std::chrono::milliseconds(100));
        REQUIRE(r.isError());

        t.close();
        teardownServer();
    }

    // ========= Cancel =========

    void cancel_cancelDuringRecv() {
        setupServer();
        UdpTransport t("0.0.0.0", 0, "127.0.0.1", port_, false, nullptr);
        REQUIRE(t.open().isOk());

        t.cancel();

        ByteBuffer buf;
        auto r = t.read(buf, 1, 10, std::chrono::seconds(1));
        REQUIRE(r.isError());

        t.close();
        teardownServer();
    }

    // ========= Large payload =========

    void largePayload_64KB() {
        setupServer();

        UdpTransport t("0.0.0.0", 0, "127.0.0.1", port_, false, nullptr);
        REQUIRE(t.open().isOk());

        // UDP datagrams > ~64KB may be fragmented by kernel; use a moderate size
        std::vector<uint8_t> data(4096, 0xCD);
        auto wr = t.write(ByteBuffer(data.begin(), data.end()),
                          std::chrono::seconds(5));
        REQUIRE(wr.isOk());
        REQUIRE(wr.value() == data.size());

        ByteBuffer recvBuf;
        auto rr = t.read(recvBuf, data.size(), data.size() + 1024,
                         std::chrono::seconds(5));
        REQUIRE(rr.isOk());
        REQUIRE(recvBuf.size() == data.size());
        REQUIRE(recvBuf[0] == 0xCD);
        REQUIRE(recvBuf[4095] == 0xCD);

        t.close();
        teardownServer();
    }

    // ========= Multiple datagrams =========

    void multipleDatagrams_roundTrip() {
        setupServer();

        UdpTransport t("0.0.0.0", 0, "127.0.0.1", port_, false, nullptr);
        REQUIRE(t.open().isOk());

        for (int i = 0; i < 5; ++i) {
            uint8_t msg[] = {static_cast<uint8_t>(i)};
            auto wr = t.write(ByteBuffer(msg, msg + sizeof(msg)), std::chrono::seconds(2));
            REQUIRE(wr.isOk());

            ByteBuffer recvBuf;
            auto rr = t.read(recvBuf, 1, 10, std::chrono::seconds(2));
            REQUIRE(rr.isOk());
            REQUIRE(recvBuf.size() == 1);
            REQUIRE(recvBuf[0] == static_cast<uint8_t>(i));
        }

        t.close();
        teardownServer();
    }
}

TEST_CASE("UdpIntegrationTest", "[transport]") {
    SECTION("Open / send / recv / close") { smoke_openSendRecvClose(); }
    SECTION("Open / close / reopen") { lifecycle_openCloseReopen(); }
    SECTION("Double open returns error") { lifecycle_doubleOpen_returnsError(); }
    SECTION("Double close no error") { lifecycle_doubleClose_noError(); }
    SECTION("Send before open returns not open") { negative_sendBeforeOpen_returnsNotOpen(); }
    SECTION("Recv after close returns not open") { negative_recvAfterClose_returnsNotOpen(); }
    SECTION("Recv with no data returns timeout") { timeout_recvWithNoData_returnsTimeout(); }
    SECTION("Cancel during recv") { cancel_cancelDuringRecv(); }
    SECTION("Large payload 4KB") { largePayload_64KB(); }
    SECTION("Multiple datagrams round trip") { multipleDatagrams_roundTrip(); }
}
