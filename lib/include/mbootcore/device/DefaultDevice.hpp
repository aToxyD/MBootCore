#pragma once

#include "mbootcore/domain/IDevice.hpp"
#include "mbootcore/domain/ITransport.hpp"
#include "mbootcore/domain/IProtocol.hpp"

namespace mbootcore {

class DefaultDevice : public IDevice {
public:
    DefaultDevice(ITransport& transport, IProtocol& protocol);

    const DeviceInfo& info() const noexcept override { return m_info; }
    IProtocol& protocol() override { return m_protocol; }

    Result<void> connect() override;
    void disconnect() noexcept override;
    bool isConnected() const noexcept override;

private:
    ITransport& m_transport;
    IProtocol& m_protocol;
    DeviceInfo m_info;
};

} // namespace mbootcore
