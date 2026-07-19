#include <catch2/catch_test_macros.hpp>
#include <mbootcore/transport/TcpTransport.hpp>
#include <mbootcore/transport/network/makeTcpBackend.hpp>
#include "TransportTestUtils.hpp"
#include <memory>

using namespace mbootcore;
using namespace mbootcore::transport;
using namespace mbootcore::test;

namespace {
    std::unique_ptr<TcpEchoServer> server_;
    uint16_t port_{0};

    void setupServer() {
        port_ = findFreePort();
        REQUIRE((port_ != 0 && port_ != 9999));
        server_ = std::make_unique<TcpEchoServer>(port_);
        server_->start();
        REQUIRE(server_->isRunning());
    }

    void teardownServer() {
        if (server_) {
            server_->stop();
            server_.reset();
        }
    }

    void lifecycle_openCloseReopen() {
        setupServer();

        {
            TcpTransport t("127.0.0.1", port_, false, nullptr);
            REQUIRE(t.open().isOk());
            t.close();
            REQUIRE(!t.isOpen());
        }

        {
            TcpTransport t2("127.0.0.1", port_, false, nullptr);
            REQUIRE(t2.open().isOk());
            REQUIRE(t2.isOpen());
            t2.close();
        }

        teardownServer();
    }

    void lifecycle_doubleOpen_returnsError() {
        setupServer();
        TcpTransport t("127.0.0.1", port_, false, nullptr);
        REQUIRE(t.open().isOk());

        auto r = t.open();
        REQUIRE(r.isError());
        REQUIRE(r.error() == ErrorCode::TransportAlreadyOpen);

        t.close();
        teardownServer();
    }

    void lifecycle_doubleClose_noError() {
        setupServer();
        TcpTransport t("127.0.0.1", port_, false, nullptr);
        REQUIRE(t.open().isOk());
        t.close();
        t.close(); // should not crash
        REQUIRE(!t.isOpen());
        teardownServer();
    }

    void lifecycle_reconnect() {
        setupServer();
        TcpTransport t("127.0.0.1", port_, false, nullptr);
        REQUIRE(t.open().isOk());
        t.close();

        auto r = t.reconnect();
        REQUIRE(r.isOk());
        REQUIRE(t.isOpen());
        t.close();
        teardownServer();
    }

    void lifecycle_closeThenOpen_sameObject() {
        setupServer();
        TcpTransport t("127.0.0.1", port_, false, nullptr);
        REQUIRE(t.open().isOk());
        t.close();
        REQUIRE(!t.isOpen());

        // Reopen the same object
        auto r = t.open();
        REQUIRE(r.isOk());
        REQUIRE(t.isOpen());

        uint8_t sendData[] = {'R', 'e', '2'};
        auto wr = t.write(ByteBuffer(sendData, sendData + sizeof(sendData)),
                          std::chrono::seconds(2));
        REQUIRE(wr.isOk());

        ByteBuffer recvBuf;
        auto rr = t.read(recvBuf, sizeof(sendData), sizeof(sendData) * 2,
                         std::chrono::seconds(2));
        REQUIRE(rr.isOk());
        REQUIRE(recvBuf.size() == sizeof(sendData));
        REQUIRE(recvBuf[0] == 'R');

        t.close();
        teardownServer();
    }

    void negative_failOpenThenRetry() {
        // First open should fail (no server on this port yet)
        uint16_t failPort = port_ = findFreePort();
        REQUIRE((failPort != 0 && failPort != 9999));

        TcpTransport t("127.0.0.1", failPort, false, nullptr);
        auto r1 = t.open();
        REQUIRE(r1.isError()); // connection refused, no server

        // Reset state
        t.close();
        REQUIRE(!t.isOpen());

        // Now start a server on the same port and retry
        server_ = std::make_unique<TcpEchoServer>(failPort);
        server_->start();
        REQUIRE(server_->isRunning());

        auto r2 = t.open();
        REQUIRE(r2.isOk());
        REQUIRE(t.isOpen());

        uint8_t d[] = {'O', 'K'};
        auto wr = t.write(ByteBuffer(d, d + sizeof(d)), std::chrono::seconds(2));
        REQUIRE(wr.isOk());

        ByteBuffer recvBuf;
        auto rr = t.read(recvBuf, sizeof(d), sizeof(d) * 2, std::chrono::seconds(2));
        REQUIRE(rr.isOk());
        REQUIRE(recvBuf[0] == 'O');

        t.close();
        teardownServer();
    }

    // ========= Negative =========

    void negative_writeBeforeOpen_returnsNotOpen() {
        TcpTransport t("127.0.0.1", port_, false, nullptr);
        auto r = t.write({0x01}, std::chrono::milliseconds(100));
        REQUIRE(r.isError());
        REQUIRE(r.error() == ErrorCode::TransportNotOpen);
    }

    void negative_readAfterClose_returnsNotOpen() {
        setupServer();
        TcpTransport t("127.0.0.1", port_, false, nullptr);
        REQUIRE(t.open().isOk());
        t.close();

        ByteBuffer buf;
        auto r = t.read(buf, 1, 10, std::chrono::milliseconds(100));
        REQUIRE(r.isError());
        REQUIRE(r.error() == ErrorCode::TransportNotOpen);
        teardownServer();
    }

    // ========= Timeout =========

    void timeout_readWithNoData_returnsTimeout() {
        setupServer();
        TcpTransport t("127.0.0.1", port_, false, nullptr);
        REQUIRE(t.open().isOk());

        ByteBuffer buf;
        auto r = t.read(buf, 1, 10, std::chrono::milliseconds(100));
        REQUIRE(r.isError());
        REQUIRE(r.error() == ErrorCode::TransportTimeout);

        t.close();
        teardownServer();
    }

    // ========= Cancel =========

    void cancel_cancelDuringRead() {
        setupServer();
        TcpTransport t("127.0.0.1", port_, false, nullptr);
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

        TcpTransport t("127.0.0.1", port_, false, nullptr);
        REQUIRE(t.open().isOk());

        std::vector<uint8_t> largeData(65536, 0xAB);
        auto wr = t.write(ByteBuffer(largeData.begin(), largeData.end()),
                          std::chrono::seconds(5));
        REQUIRE(wr.isOk());
        REQUIRE(wr.value() == largeData.size());

        ByteBuffer recvBuf;
        auto rr = t.read(recvBuf, largeData.size(), largeData.size() + 1024,
                         std::chrono::seconds(5));
        REQUIRE(rr.isOk());
        REQUIRE(recvBuf.size() == largeData.size());
        REQUIRE(recvBuf[0] == 0xAB);
        REQUIRE(recvBuf[65535] == 0xAB);

        t.close();
        teardownServer();
    }
}

TEST_CASE("TcpIntegrationTest", "[transport]") {
    SECTION("Open / close / reopen") { lifecycle_openCloseReopen(); }
    SECTION("Double open returns error") { lifecycle_doubleOpen_returnsError(); }
    SECTION("Double close no error") { lifecycle_doubleClose_noError(); }
    SECTION("Reconnect") { lifecycle_reconnect(); }
    SECTION("Close then open same object") { lifecycle_closeThenOpen_sameObject(); }
    SECTION("Fail open then retry") { negative_failOpenThenRetry(); }
    SECTION("Write before open returns not open") { negative_writeBeforeOpen_returnsNotOpen(); }
    SECTION("Read after close returns not open") { negative_readAfterClose_returnsNotOpen(); }
    SECTION("Read with no data returns timeout") { timeout_readWithNoData_returnsTimeout(); }
    SECTION("Cancel during read") { cancel_cancelDuringRead(); }
    SECTION("Large payload 64KB") { largePayload_64KB(); }
}
