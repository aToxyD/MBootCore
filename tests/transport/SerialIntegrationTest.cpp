#include <catch2/catch_test_macros.hpp>
#include <mbootcore/transport/serial/makeSerialBackend.hpp>
#include "MockSerialBackend.hpp"
#include <memory>

using namespace mbootcore;
using namespace mbootcore::transport;
using namespace mbootcore::transport::serial;

namespace {
    std::unique_ptr<MockSerialBackend> mock_;

    void init() {
        mock_ = std::make_unique<MockSerialBackend>();
    }

    void cleanup() {
        if (mock_) {
            mock_->close();
            mock_.reset();
        }
    }

    // ========= Smoke =========

    void smoke_openWriteReadClose() {
        REQUIRE(mock_->isAvailable());

        auto r = mock_->open("/dev/ttyUSB0", 115200, 8, 1, "none", "none");
        REQUIRE(r.isOk());
        REQUIRE(mock_->isOpen());
        REQUIRE(mock_->lastPortName() == "/dev/ttyUSB0");
        REQUIRE(mock_->lastBaudRate() == 115200);

        uint8_t sendData[] = {'S', 'e', 'r', 'i', 'a', 'l'};
        auto wr = mock_->write(sendData, sizeof(sendData), std::chrono::seconds(1));
        REQUIRE(wr.isOk());
        REQUIRE(wr.value() == sizeof(sendData));
        REQUIRE(mock_->totalBytesWritten() == sizeof(sendData));

        uint8_t rd[] = {0x01, 0x02};
        mock_->setReadData(ByteBuffer(rd, rd + sizeof(rd)));

        uint8_t buf[16] = {};
        auto rr = mock_->read(buf, sizeof(buf), std::chrono::seconds(1));
        REQUIRE(rr.isOk());
        REQUIRE(rr.value() == sizeof(rd));
        REQUIRE(buf[0] == 0x01);

        mock_->close();
        REQUIRE(!mock_->isOpen());
    }

    // ========= Lifecycle =========

    void lifecycle_openCloseReopen() {
        REQUIRE(mock_->open("COM1", 9600, 8, 1, "none", "none").isOk());
        mock_->close();
        REQUIRE(!mock_->isOpen());

        REQUIRE(mock_->open("COM2", 115200, 8, 1, "none", "none").isOk());
        REQUIRE(mock_->isOpen());
        REQUIRE(mock_->lastPortName() == "COM2");
    }

    void lifecycle_doubleOpen_returnsError() {
        mock_->open("COM1", 9600, 8, 1, "none", "none");
        auto r = mock_->open("COM1", 115200, 8, 1, "none", "none");
        REQUIRE(r.isError());
    }

    void lifecycle_doubleClose_noError() {
        mock_->open("COM1", 9600, 8, 1, "none", "none");
        mock_->close();
        mock_->close();
        REQUIRE(!mock_->isOpen());
    }

    void lifecycle_reopenAfterClose() {
        mock_->open("COM1", 9600, 8, 1, "none", "none");
        uint8_t d[] = {0xAA};
        mock_->write(d, 1, std::chrono::seconds(1));
        REQUIRE(mock_->totalBytesWritten() == 1);
        mock_->close();

        mock_->open("COM1", 115200, 8, 1, "none", "none");
        // Should start fresh
        REQUIRE(mock_->totalBytesWritten() == 1);
        REQUIRE(mock_->writes().size() == 1);
    }

    // ========= Negative =========

    void negative_writeBeforeOpen_returnsNotOpen() {
        mock_->close();
        uint8_t d[] = {0x01};
        auto r = mock_->write(d, 1, std::chrono::seconds(1));
        REQUIRE(r.isError());
        REQUIRE(r.error() == ErrorCode::TransportNotOpen);
    }

    void negative_readAfterClose_returnsNotOpen() {
        mock_->open("COM1", 9600, 8, 1, "none", "none");
        mock_->close();
        uint8_t buf[4] = {};
        auto r = mock_->read(buf, sizeof(buf), std::chrono::seconds(1));
        REQUIRE(r.isError());
        REQUIRE(r.error() == ErrorCode::TransportNotOpen);
    }

    void negative_failOnOpen() {
        mock_->setFailOnOpen(true);
        auto r = mock_->open("COM1", 9600, 8, 1, "none", "none");
        REQUIRE(r.isError());
        REQUIRE(!mock_->isOpen());
    }

    void negative_failOnWrite() {
        mock_->open("COM1", 9600, 8, 1, "none", "none");
        mock_->setFailOnWrite(true);
        uint8_t d[] = {0x01};
        auto r = mock_->write(d, 1, std::chrono::seconds(1));
        REQUIRE(r.isError());
        REQUIRE(r.error() == ErrorCode::TransportWriteFailed);
    }

    void negative_failOnRead() {
        mock_->open("COM1", 9600, 8, 1, "none", "none");
        mock_->setFailOnRead(true);
        uint8_t buf[4] = {};
        auto r = mock_->read(buf, sizeof(buf), std::chrono::seconds(1));
        REQUIRE(r.isError());
        REQUIRE(r.error() == ErrorCode::TransportReadFailed);
    }

    // ========= Timeout =========

    void timeout_readTimeout_whenQueueEmpty() {
        mock_->open("COM1", 9600, 8, 1, "none", "none");
        mock_->setReadTimeout(true);
        uint8_t buf[4] = {};
        auto r = mock_->read(buf, sizeof(buf), std::chrono::milliseconds(100));
        REQUIRE(r.isError());
        REQUIRE(r.error() == ErrorCode::TransportTimeout);
    }

    // ========= Cancel =========

    void cancel_cancelDuringRead() {
        mock_->open("COM1", 9600, 8, 1, "none", "none");
        REQUIRE(!mock_->wasCancelled());
        mock_->cancel();
        REQUIRE(mock_->wasCancelled());

        uint8_t buf[4] = {};
        auto r = mock_->read(buf, sizeof(buf), std::chrono::seconds(1));
        REQUIRE(r.isError());
    }

    void cancel_cancelDuringWrite() {
        mock_->open("COM1", 9600, 8, 1, "none", "none");
        mock_->cancel();
        uint8_t d[] = {0x01};
        auto r = mock_->write(d, 1, std::chrono::seconds(1));
        REQUIRE(r.isError());
    }

    // ========= Config =========

    void config_differentBaudRates() {
        mock_->open("COM1", 9600, 8, 1, "none", "none");
        REQUIRE(mock_->lastBaudRate() == 9600);
        mock_->close();

        mock_->open("COM1", 115200, 8, 1, "none", "none");
        REQUIRE(mock_->lastBaudRate() == 115200);
        mock_->close();

        mock_->open("COM1", 300, 8, 1, "none", "none");
        REQUIRE(mock_->lastBaudRate() == 300);
    }

    // ========= Flush =========

    void flush_success() {
        mock_->open("COM1", 9600, 8, 1, "none", "none");
        auto r = mock_->flush();
        REQUIRE(r.isOk());
    }

    void flush_failure() {
        mock_->open("COM1", 9600, 8, 1, "none", "none");
        mock_->setFailOnFlush(true);
        auto r = mock_->flush();
        REQUIRE(r.isError());
        REQUIRE(r.error() == ErrorCode::TransportWriteFailed);
    }

    void flush_notOpen_returnsNotOpen() {
        mock_->close();
        auto r = mock_->flush();
        REQUIRE(r.isError());
        REQUIRE(r.error() == ErrorCode::TransportNotOpen);
    }

    // ========= Call Records =========

    void callRecords_order() {
        mock_->open("COM1", 9600, 8, 1, "none", "none");
        uint8_t d[] = {0x01, 0x02};
        mock_->write(d, 2, std::chrono::seconds(1));
        mock_->flush();
        mock_->close();

        auto recs = mock_->records();
        REQUIRE(recs.size() == 4);
        REQUIRE(static_cast<int>(recs[0].type) == static_cast<int>(MockSerialBackend::CallRecord::Open));
        REQUIRE(static_cast<int>(recs[1].type) == static_cast<int>(MockSerialBackend::CallRecord::Write));
        REQUIRE(static_cast<int>(recs[2].type) == static_cast<int>(MockSerialBackend::CallRecord::Flush));
        REQUIRE(static_cast<int>(recs[3].type) == static_cast<int>(MockSerialBackend::CallRecord::Close));
    }
}

TEST_CASE("SerialIntegrationTest", "[transport]") {
    init();
    SECTION("Open / write / read / close") { smoke_openWriteReadClose(); }
    SECTION("Open / close / reopen") { lifecycle_openCloseReopen(); }
    SECTION("Double open returns error") { lifecycle_doubleOpen_returnsError(); }
    SECTION("Double close no error") { lifecycle_doubleClose_noError(); }
    SECTION("Reopen after close") { lifecycle_reopenAfterClose(); }
    SECTION("Write before open returns not open") { negative_writeBeforeOpen_returnsNotOpen(); }
    SECTION("Read after close returns not open") { negative_readAfterClose_returnsNotOpen(); }
    SECTION("Fail on open") { negative_failOnOpen(); }
    SECTION("Fail on write") { negative_failOnWrite(); }
    SECTION("Fail on read") { negative_failOnRead(); }
    SECTION("Read timeout when queue empty") { timeout_readTimeout_whenQueueEmpty(); }
    SECTION("Cancel during read") { cancel_cancelDuringRead(); }
    SECTION("Cancel during write") { cancel_cancelDuringWrite(); }
    SECTION("Different baud rates") { config_differentBaudRates(); }
    SECTION("Flush success") { flush_success(); }
    SECTION("Flush failure") { flush_failure(); }
    SECTION("Flush not open returns not open") { flush_notOpen_returnsNotOpen(); }
    SECTION("Call records order") { callRecords_order(); }
    cleanup();
}
