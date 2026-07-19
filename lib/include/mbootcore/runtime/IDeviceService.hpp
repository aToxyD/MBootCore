#pragma once

#include <mbootcore/domain/Error.hpp>
#include <mbootcore/discovery/DiscoveryTypes.hpp>

#include <chrono>
#include <memory>

namespace mbootcore {

namespace session { class DeviceSession; }

namespace runtime {

class IDeviceService {
public:
    virtual ~IDeviceService() = default;

    virtual Result<std::vector<discovery::DeviceDescriptor>> discover(
        std::chrono::milliseconds timeout = std::chrono::seconds(5)) = 0;

    virtual Result<discovery::DeviceDescriptor> probe(
        const discovery::DeviceDescriptor& hint) = 0;

    virtual Result<void> connect(
        const discovery::DeviceDescriptor& descriptor) = 0;

    virtual void disconnect() noexcept = 0;

    virtual Result<void> reconnect() = 0;

    virtual bool isConnected() const noexcept = 0;

    virtual session::DeviceSession* activeSession() const noexcept = 0;
};

} // namespace runtime
} // namespace mbootcore
