#pragma once

#include <mbootcore/transport/TransportTypes.hpp>
#include <mbootcore/domain/ITransport.hpp>
#include <mbootcore/domain/ILogger.hpp>
#include <memory>
#include <string>

namespace mbootcore {
namespace transport {

/// @brief Configuration for creating a USB transport via TransportFactory.
struct UsbFactoryConfig {
    uint16_t vendorId{0x05C6};
    uint16_t productId{0x9008};
    int interfaceNumber{0};
    int bulkInEndpoint{0x81};
    int bulkOutEndpoint{0x01};
};

/// @brief Configuration for creating a serial transport via TransportFactory.
struct SerialFactoryConfig {
    std::string portName;
    int baudRate{115200};
    int dataBits{8};
    int stopBits{1};
    std::string parity{"none"};
    std::string flowControl{"none"};
};

/// @brief Configuration for creating a TCP transport via TransportFactory.
struct TcpFactoryConfig {
    std::string host;
    uint16_t port{0};
    bool keepAlive{true};
};

/// @brief Configuration for creating a UDP transport via TransportFactory.
struct UdpFactoryConfig {
    std::string localAddress;
    uint16_t localPort{0};
    std::string remoteAddress;
    uint16_t remotePort{0};
    bool broadcast{false};
};

/**
 * @brief Factory for creating ITransport instances.
 *
 * Creates transport wrappers (SerialTransport, TcpTransport, etc.)
 * pre-configured for common boot-mode scenarios.
 *
 * Note: For direct backend access, use the make*Backend() free functions
 * instead (makeSerialBackend, makeTcpBackend, etc.).
 */
class TransportFactory {
public:
    static std::unique_ptr<ITransport> createMock();
    static std::unique_ptr<ITransport> createVirtual();
    static std::unique_ptr<ITransport> createUSB(const UsbFactoryConfig& config = {}, ILogger* logger = nullptr);
    static std::unique_ptr<ITransport> createSerial(const SerialFactoryConfig& config, ILogger* logger = nullptr);
    static std::unique_ptr<ITransport> createTCP(const TcpFactoryConfig& config, ILogger* logger = nullptr);
    static std::unique_ptr<ITransport> createUDP(const UdpFactoryConfig& config = {}, ILogger* logger = nullptr);
};

} // namespace transport
} // namespace mbootcore
