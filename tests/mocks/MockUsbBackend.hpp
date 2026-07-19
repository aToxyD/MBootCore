#pragma once

#include <mbootcore/transport/usb/UsbBackend.hpp>
#include <deque>
#include <vector>
#include <cstring>

namespace mbootcore {

class MockUsbBackend : public transport::usb::UsbBackend {
public:
    struct CallRecord {
        enum Type { Open, Close, BulkRead, BulkWrite, ControlRead, ControlWrite,
                     ClaimInterface, ReleaseInterface, ResetDevice, ResetPipe,
                     AbortPipe };
        Type type;
        uint8_t endpoint{0};
        size_t size{0};
    };

    bool isAvailable() const noexcept override { return true; }

    Result<void> open(uint16_t vendorId, uint16_t productId,
                      int interfaceNumber) override {
        m_records.push_back({CallRecord::Open, 0, 0});
        if (m_failOnOpen) return Result<void>::Error(ErrorCode::TransportError);
        m_isOpen = true;
        return Result<void>::Ok();
    }

    void close() noexcept override {
        m_records.push_back({CallRecord::Close, 0, 0});
        m_isOpen = false;
    }

    bool isOpen() const noexcept override { return m_isOpen; }

    Result<size_t> bulkRead(uint8_t endpoint, uint8_t* buffer, size_t size,
                            std::chrono::milliseconds) override {
        m_records.push_back({CallRecord::BulkRead, endpoint, size});
        if (m_failOnRead) return Result<size_t>::Error(ErrorCode::TransportReadFailed);
        if (!m_readQueue.empty()) {
            auto& data = m_readQueue.front();
            size_t copySize = std::min(data.size(), size);
            std::memcpy(buffer, data.data(), copySize);
            m_readQueue.pop_front();
            m_totalBytesRead += copySize;
            return Result<size_t>::Ok(copySize);
        }
        if (m_readTimeout) return Result<size_t>::Error(ErrorCode::TransportTimeout);
        return Result<size_t>::Ok(0);
    }

    Result<size_t> bulkWrite(uint8_t endpoint, const uint8_t* data, size_t size,
                             std::chrono::milliseconds) override {
        m_records.push_back({CallRecord::BulkWrite, endpoint, size});
        if (m_failOnWrite) return Result<size_t>::Error(ErrorCode::TransportWriteFailed);
        m_writes.emplace_back(data, data + size);
        m_totalBytesWritten += size;
        return Result<size_t>::Ok(size);
    }

    Result<size_t> controlRead(uint8_t, uint8_t, uint16_t, uint16_t,
                                uint8_t*, size_t, std::chrono::milliseconds) override {
        m_records.push_back({CallRecord::ControlRead, 0, 0});
        return Result<size_t>::Error(ErrorCode::TransportBackendUnavailable);
    }

    Result<size_t> controlWrite(uint8_t, uint8_t, uint16_t, uint16_t,
                                 const uint8_t*, size_t, std::chrono::milliseconds) override {
        m_records.push_back({CallRecord::ControlWrite, 0, 0});
        return Result<size_t>::Error(ErrorCode::TransportBackendUnavailable);
    }

    Result<void> claimInterface(int) override {
        m_records.push_back({CallRecord::ClaimInterface, 0, 0});
        return Result<void>::Ok();
    }

    Result<void> releaseInterface(int) override {
        m_records.push_back({CallRecord::ReleaseInterface, 0, 0});
        return Result<void>::Ok();
    }

    Result<void> resetDevice() override {
        m_records.push_back({CallRecord::ResetDevice, 0, 0});
        return Result<void>::Ok();
    }

    Result<void> resetPipe(uint8_t endpoint) override {
        m_records.push_back({CallRecord::ResetPipe, endpoint, 0});
        if (m_failOnReset) return Result<void>::Error(ErrorCode::TransportError);
        return Result<void>::Ok();
    }

    Result<void> abortPipe(uint8_t endpoint) override {
        m_records.push_back({CallRecord::AbortPipe, endpoint, 0});
        return Result<void>::Ok();
    }

    Result<transport::usb::UsbDeviceInfo> deviceInfo() const override {
        return Result<transport::usb::UsbDeviceInfo>::Ok({});
    }

    std::string backendName() const noexcept override { return "MockUsbBackend"; }

    void setLogger(ILogger*) noexcept override {}

    // Mock control
    void setReadData(const ByteBuffer& data) { m_readQueue.push_back(data); }
    void setFailOnOpen(bool v) { m_failOnOpen = v; }
    void setFailOnWrite(bool v) { m_failOnWrite = v; }
    void setFailOnRead(bool v) { m_failOnRead = v; }
    void setReadTimeout(bool v) { m_readTimeout = v; }

    const std::vector<CallRecord>& records() const { return m_records; }
    const std::vector<ByteBuffer>& writes() const { return m_writes; }
    size_t totalBytesRead() const { return m_totalBytesRead; }
    size_t totalBytesWritten() const { return m_totalBytesWritten; }
    void clearRecords() { m_records.clear(); m_writes.clear(); }

private:
    bool m_isOpen{false};
    bool m_failOnOpen{false};
    bool m_failOnWrite{false};
    bool m_failOnRead{false};
    bool m_failOnReset{false};
    bool m_readTimeout{false};
    size_t m_totalBytesRead{0};
    size_t m_totalBytesWritten{0};
    std::deque<ByteBuffer> m_readQueue;
    std::vector<CallRecord> m_records;
    std::vector<ByteBuffer> m_writes;
};

} // namespace mbootcore
