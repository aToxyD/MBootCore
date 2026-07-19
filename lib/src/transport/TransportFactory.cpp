#include <mbootcore/transport/TransportFactory.hpp>
#include <mbootcore/transport/UsbTransport.hpp>
#include <mbootcore/transport/SerialTransport.hpp>
#include <mbootcore/transport/TcpTransport.hpp>
#include <mbootcore/transport/UdpTransport.hpp>
#include <mbootcore/transport/VirtualTransports.hpp>

namespace mbootcore {
namespace transport {

std::unique_ptr<ITransport> TransportFactory::createMock() {
    return std::make_unique<VirtualUsbTransport>();
}

std::unique_ptr<ITransport> TransportFactory::createVirtual() {
    return std::make_unique<VirtualUsbTransport>();
}

std::unique_ptr<ITransport> TransportFactory::createUSB(const UsbFactoryConfig& config, ILogger* logger) {
    return std::make_unique<UsbTransport>(config.vendorId, config.productId,
        config.interfaceNumber, config.bulkInEndpoint, config.bulkOutEndpoint, logger);
}

std::unique_ptr<ITransport> TransportFactory::createSerial(const SerialFactoryConfig& config, ILogger* logger) {
    return std::make_unique<SerialTransport>(config.portName, config.baudRate,
        config.dataBits, config.stopBits, config.parity, config.flowControl, logger);
}

std::unique_ptr<ITransport> TransportFactory::createTCP(const TcpFactoryConfig& config, ILogger* logger) {
    return std::make_unique<TcpTransport>(config.host, config.port, config.keepAlive, logger);
}

std::unique_ptr<ITransport> TransportFactory::createUDP(const UdpFactoryConfig& config, ILogger* logger) {
    return std::make_unique<UdpTransport>(config.localAddress, config.localPort,
        config.remoteAddress, config.remotePort, config.broadcast, logger);
}

} // namespace transport
} // namespace mbootcore
