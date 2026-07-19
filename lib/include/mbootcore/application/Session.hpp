#pragma once

#include "mbootcore/domain/IProtocol.hpp"
#include "mbootcore/domain/ITransport.hpp"
#include "mbootcore/domain/ILogger.hpp"
#include "mbootcore/domain/IDevice.hpp"

#include "mbootcore/loader/LoaderManager.hpp"

#include <memory>
#include <string>

namespace mbootcore {

class Session {
public:
    struct Config {
        uint16_t vendorId{0x05C6};
        uint16_t productId{0x9008};
        std::string searchPath{"./Loaders"};
    };

    Session();
    explicit Session(Config config);
    ~Session() noexcept;

    void setLogger(std::unique_ptr<ILogger> logger);
    void setTransport(std::unique_ptr<ITransport> transport);
    void setProtocol(std::unique_ptr<IProtocol> protocol);

    Result<void> connect();
    void disconnect();

    Result<void> detectDevice();
    Result<void> uploadLoader(const std::string& loaderPath = "");
    Result<void> resetDevice();

    bool isConnected() const noexcept;
    ILogger& logger() noexcept { return *m_logger; }

private:
    Config m_config;
    std::unique_ptr<ILogger> m_logger;
    std::unique_ptr<ITransport> m_transport;
    std::unique_ptr<IProtocol> m_protocol;
    std::unique_ptr<IDevice> m_device;
    std::unique_ptr<LoaderManager> m_loaderManager;
};

} // namespace mbootcore
