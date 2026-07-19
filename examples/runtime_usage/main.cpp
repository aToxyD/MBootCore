#include <mbootcore/MBootCore.hpp>
#include <mbootcore/runtime/RuntimeFactory.hpp>
#include <mbootcore/runtime/RuntimeBuilder.hpp>
#include <mbootcore/runtime/Runtime.hpp>
#include <mbootcore/runtime/RuntimeConfig.hpp>
#include <mbootcore/runtime/RuntimeObserver.hpp>
#include <mbootcore/logging/ConsoleLogger.hpp>
#include <mbootcore/logging/FileLogger.hpp>
#include <sdk/Version.hpp>
#include <iostream>
#include <cstdlib>
#include <memory>
#include <thread>
#include <sstream>

using namespace mbootcore;

class ExampleObserver : public runtime::RuntimeObserver {
public:
    void onRuntimeEvent(const runtime::RuntimeEvent& event) override {
        std::ostringstream oss;
        oss << "[EVENT] type=" << static_cast<int>(event.type)
            << " source=" << event.source
            << " success=" << (event.success ? "yes" : "no")
            << " msg=" << event.message;
        std::cout << oss.str() << std::endl;
    }

    void onStatisticsUpdated(const runtime::RuntimeStatistics& stats) override {
        std::cout << "[STATS] devices=" << stats.devicesConnected
                  << " jobs=" << stats.jobsExecuted
                  << " errors=" << stats.totalErrors << std::endl;
    }

    void onHealthChanged(const runtime::RuntimeHealth& health) override {
        std::cout << "[HEALTH] sessions=" << health.activeSessions
                  << " workflows=" << health.activeWorkflows
                  << " uptime=" << health.uptimeSeconds << "s" << std::endl;
    }
};

int main() {
    auto logger = std::make_shared<ConsoleLogger>();
    logger->info("example", "MBootCore Runtime Full Lifecycle Example\n");

    // Configure runtime
    runtime::RuntimeConfig config;
    config.logging.enabled = true;
    config.logging.level = "debug";
    config.logging.consoleOutput = true;
    config.maxRetries = 5;
    config.autoRecovery = true;
    config.enableStatistics = true;
    config.enableVendorRuntime = true;
    config.discoverTimeout = std::chrono::seconds(5);

    config.transport.timeout = std::chrono::seconds(10);
    config.transport.retryCount = 3;
    config.transport.reconnect = true;
    config.transport.bufferSize = 131072;

    config.session.connectTimeout = std::chrono::seconds(10);
    config.session.operationTimeout = std::chrono::seconds(60);
    config.session.enableStatistics = true;
    config.session.enableHistory = true;

    // Build runtime
    auto runtime = runtime::RuntimeBuilder()
                       .withConfig(config)
                       .withLogger(std::make_unique<ConsoleLogger>(LogLevel::Debug))
                       .build();

    // Attach observer
    ExampleObserver observer;
    runtime.addObserver(&observer);

    // Initialize
    logger->info("example", "Initializing runtime...");
    auto result = runtime.initialize();
    if (!result.isOk()) {
        std::cerr << "Init failed: " << toString(result.error()) << std::endl;
        return EXIT_FAILURE;
    }
    logger->info("example", "Runtime initialized");

    // Version info
    auto versionInfo = sdk::getVersionInfo();
    logger->info("example", "Version: " + versionInfo.sdkVersion.toString());
    logger->info("example", "Core: " + versionInfo.coreVersion.toString());
    logger->info("example",
                 "Platform: " + versionInfo.platformInfo.osName + " " +
                     versionInfo.platformInfo.osVersion);
    logger->info("example",
                 "Compiler: " + versionInfo.compilerInfo.compilerName + " " +
                     versionInfo.compilerInfo.compilerVersion);

    // Hardware info
    logger->info("example", "\n--- Hardware Report ---");
    auto hwReport = runtime.hardwareReport();
    for (const auto& dev : hwReport.detectedDevices) {
        logger->info("example",
                     "  HW: " + dev.name + " (" + dev.vendor +
                         ") via " + dev.transportType);
    }
    for (const auto& w : hwReport.warnings) {
        logger->warn("example", "  HW Warn: " + w);
    }

    auto usbDevices = runtime.usbDevices();
    logger->info("example",
                 "USB devices: " + std::to_string(usbDevices.size()));

    auto serialPorts = runtime.serialPorts();
    logger->info("example",
                 "Serial ports: " + std::to_string(serialPorts.size()));

    // Capabilities
    logger->info("example", "\n--- Capabilities ---");
    auto caps = runtime.capabilities();
    for (const auto& cap : caps) {
        logger->info("example", "  - " + cap);
    }

    // Discovery
    logger->info("example", "\n--- Device Discovery ---");
    auto devices = runtime.discover(std::chrono::seconds(3));
    if (devices.isOk()) {
        logger->info("example",
                     "Found " + std::to_string(devices.value().size()) + " device(s)");
        for (const auto& d : devices.value()) {
            logger->info("example",
                         "  " + d.friendlyName + " [vendor=" +
                             std::to_string(static_cast<int>(d.vendor)) + "]");
        }
    } else {
        logger->info("example",
                     "Discovery: " + std::string(toString(devices.error())));
    }

    // Plugin listing
    logger->info("example", "\n--- Plugins ---");
    auto plugins = runtime.listPlugins();
    logger->info("example",
                 "Loaded plugins: " + std::to_string(plugins.size()));
    for (const auto& p : plugins) {
        logger->info("example", "  - " + p);
    }

    // Statistics & Health
    logger->info("example", "\n--- Runtime State ---");
    auto stats = runtime.statistics();
    auto health = runtime.health();
    logger->info("example",
                 "Uptime: " + std::to_string(stats.uptimeSeconds) + "s");
    logger->info("example",
                 "Active sessions: " + std::to_string(health.activeSessions));
    logger->info("example",
                 "Memory usage: " + std::to_string(health.memoryUsageBytes) + " bytes");

    // Pause/Resume demo
    logger->info("example", "\n--- Pause/Resume ---");
    runtime.pause();
    logger->info("example", "Runtime paused");
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    runtime.resume();
    logger->info("example", "Runtime resumed");

    // Cancel demo
    runtime.cancel();
    logger->info("example", "Runtime operations cancelled");

    // Reset demo
    auto resetResult = runtime.reset();
    if (resetResult.isOk()) {
        logger->info("example", "Runtime reset completed");
    }

    // Shutdown
    runtime.removeObserver(&observer);
    runtime.shutdown();
    logger->info("example", "\nRuntime shut down cleanly");

    return EXIT_SUCCESS;
}
