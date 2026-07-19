#include <mbootcore/MBootCore.hpp>
#include <mbootcore/runtime/RuntimeFactory.hpp>
#include <mbootcore/runtime/Runtime.hpp>
#include <mbootcore/logging/ConsoleLogger.hpp>
#include <sdk/Version.hpp>
#include <iostream>
#include <cstdlib>

int main() {
    using namespace mbootcore;

    auto logger = std::make_shared<ConsoleLogger>(LogLevel::Debug);

    auto runtime = runtime::RuntimeFactory::createDefault();

    runtime::RuntimeCallbacks cb;
    cb.onLog = [&](const std::string& msg) { logger->info("runtime", msg); };
    cb.onError = [&](const std::string& msg) { logger->error("runtime", msg); };
    cb.onStatus = [&](const std::string& msg) { logger->info("runtime", msg); };
    cb.onDeviceConnected =
        [&](const discovery::DeviceDescriptor& dev) {
            logger->info("runtime", "Device connected: " + dev.friendlyName);
        };
    cb.onProgress = [&](const std::string& op, double pct) {
        logger->info("runtime", op + " " + std::to_string(static_cast<int>(pct)) + "%");
    };
    runtime.setCallbacks(cb);

    auto result = runtime.initialize();
    if (!result.isOk()) {
        std::cerr << "Failed to initialize runtime: "
                  << toString(result.error()) << std::endl;
        return EXIT_FAILURE;
    }

    logger->info("example", "MBootCore Runtime initialized successfully");

    auto ver = sdk::getSDKVersion();
    logger->info("example", "SDK Version: " + ver.toString());

    auto health = runtime.health();
    logger->info("example",
                 "Active sessions: " + std::to_string(health.activeSessions));

    auto devices = runtime.discover(std::chrono::seconds(3));
    if (devices.isOk()) {
        logger->info("example",
                     "Discovered " + std::to_string(devices.value().size()) + " device(s)");
        for (const auto& d : devices.value()) {
            logger->info("example",
                         "  - " + d.friendlyName +
                             " [vendor=" + std::to_string(static_cast<int>(d.vendor)) +
                             " protocol=" + std::to_string(static_cast<int>(d.protocolHint)) +
                             "]");
        }
    } else {
        logger->warn("example",
                     "Discovery returned: " + std::string(toString(devices.error())));
    }

    auto stats = runtime.statistics();
    logger->info("example",
                 "Runtime uptime: " + std::to_string(stats.uptimeSeconds) + "s");

    runtime.shutdown();
    logger->info("example", "Runtime shut down cleanly");
    return EXIT_SUCCESS;
}
