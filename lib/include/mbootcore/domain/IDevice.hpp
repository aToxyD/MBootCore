#pragma once

#include "Types.hpp"
#include "IProtocol.hpp"

namespace mbootcore {

class IDevice {
public:
    virtual ~IDevice() = default;

    virtual const DeviceInfo& info() const noexcept = 0;
    virtual IProtocol& protocol() = 0;
    virtual Result<void> connect() = 0;
    virtual void disconnect() noexcept = 0;
    virtual bool isConnected() const noexcept = 0;
};

} // namespace mbootcore
