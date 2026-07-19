#include "mbootcore/device/DefaultDevice.hpp"

namespace mbootcore {

DefaultDevice::DefaultDevice(ITransport& transport, IProtocol& protocol)
    : m_transport(transport)
    , m_protocol(protocol) {}

Result<void> DefaultDevice::connect() {
    return m_protocol.handshake();
}

void DefaultDevice::disconnect() noexcept {
    m_transport.close();
}

bool DefaultDevice::isConnected() const noexcept {
    return m_transport.isOpen();
}

} // namespace mbootcore
