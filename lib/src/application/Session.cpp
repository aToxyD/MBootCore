#include "mbootcore/application/Session.hpp"
#include "mbootcore/core/protocols/sahara/SaharaProtocol.hpp"
#include "mbootcore/device/DefaultDevice.hpp"
#include "mbootcore/transport/TransportFactory.hpp"
#include "mbootcore/logging/ConsoleLogger.hpp"

namespace mbootcore {

Session::Session()
    : m_logger(std::make_unique<ConsoleLogger>())
    , m_loaderManager(std::make_unique<LoaderManager>(*m_logger)) {}

Session::Session(Config config)
    : m_config(std::move(config))
    , m_logger(std::make_unique<ConsoleLogger>())
    , m_loaderManager(std::make_unique<LoaderManager>(*m_logger)) {}

Session::~Session() noexcept { disconnect(); }

void Session::setLogger(std::unique_ptr<ILogger> logger) {
    m_logger = std::move(logger);
    m_loaderManager = std::make_unique<LoaderManager>(*m_logger);
}

void Session::setTransport(std::unique_ptr<ITransport> transport) {
    m_transport = std::move(transport);
}

void Session::setProtocol(std::unique_ptr<IProtocol> protocol) {
    m_protocol = std::move(protocol);
}

Result<void> Session::connect() {
    m_logger->info("Session", "Connecting to device...");

    if (!m_transport) {
        transport::UsbFactoryConfig usbConfig;
        usbConfig.vendorId = m_config.vendorId;
        usbConfig.productId = m_config.productId;
        m_transport = transport::TransportFactory::createUSB(usbConfig, m_logger.get());
    }

    auto openResult = m_transport->open();
    if (!openResult.isOk()) {
        return openResult;
    }

    return {};
}

void Session::disconnect() {
    m_device.reset();
    m_protocol.reset();
    if (m_transport) {
        m_transport->close();
        m_transport.reset();
    }
    m_logger->info("Session", "Disconnected");
}

Result<void> Session::detectDevice() {
    m_logger->info("Session", "Detecting device...");
    if (!m_transport || !m_transport->isOpen()) {
        return ErrorCode::DeviceDisconnected;
    }

    if (!m_protocol) {
        m_protocol = std::make_unique<SaharaProtocol>(*m_transport, *m_logger);
    }

    m_device = std::make_unique<DefaultDevice>(*m_transport, *m_protocol);

    return m_device->connect();
}

Result<void> Session::uploadLoader(const std::string& loaderPath) {
    m_logger->info("Session", "Uploading loader...");

    if (!m_device || !m_device->isConnected()) {
        return ErrorCode::DeviceDisconnected;
    }

    if (!loaderPath.empty()) {
        auto loaderResult = m_loaderManager->loadFromFile(loaderPath);
        if (!loaderResult.isOk()) {
            return loaderResult.error();
        }
        auto& loader = *loaderResult.value();

        auto& proto = m_device->protocol();
        return proto.uploadProgrammer(loader.data());
    }

    return {};
}

Result<void> Session::resetDevice() {
    m_logger->info("Session", "Resetting device...");
    if (m_device) {
        return m_device->protocol().reset();
    }
    return ErrorCode::DeviceDisconnected;
}

bool Session::isConnected() const noexcept {
    return m_device && m_device->isConnected();
}

} // namespace mbootcore
