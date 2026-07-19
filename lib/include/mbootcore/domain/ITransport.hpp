#pragma once

#include "Types.hpp"
#include "Error.hpp"
#include "TransportTypes.hpp"

#include <chrono>

namespace mbootcore {

class ITransport {
public:
    virtual ~ITransport() = default;

    // === Core I/O ===
    virtual Result<void> open() = 0;
    virtual void close() noexcept = 0;
    virtual bool isOpen() const noexcept = 0;
    virtual Result<size_t> write(const ByteBuffer& data,
                                 std::chrono::milliseconds timeout) = 0;
    virtual Result<size_t> read(ByteBuffer& buffer,
                                size_t minBytes,
                                size_t maxBytes,
                                std::chrono::milliseconds timeout) = 0;
    virtual Result<void> sendZLP(std::chrono::milliseconds timeout) = 0;
    virtual std::string_view name() const noexcept = 0;
    virtual void cancel() noexcept = 0;

    // === Identification & Capabilities ===
    virtual transport::TransportType transportType() const noexcept = 0;
    virtual transport::TransportCapability capabilities() const noexcept = 0;

    // === State & Statistics ===
    virtual transport::TransportState state() const noexcept {
        return isOpen() ? transport::TransportState::Open
                        : transport::TransportState::Closed;
    }
    virtual transport::TransportStatistics statistics() const {
        return {};
    }

    // === Configuration ===
    virtual const transport::TransportConfig& config() const = 0;
    virtual void setConfig(const transport::TransportConfig& cfg) = 0;

    // === I/O Control ===
    virtual Result<void> flush() {
        return {};
    }

    // === Lifecycle ===
    virtual Result<void> reset() {
        close();
        return open();
    }
    virtual Result<void> reconnect() {
        close();
        return open();
    }

    // === Observability ===
    virtual transport::TransportProgress progress() const {
        return {};
    }
    virtual transport::TransportEndpoint endpoint() const {
        return {};
    }
};

} // namespace mbootcore
