#include <mbootcore/runtime/RuntimeFactory.hpp>
#include <mbootcore/logging/ConsoleLogger.hpp>
#include <mbootcore/logging/NullLogger.hpp>

namespace mbootcore {
namespace runtime {

Runtime RuntimeFactory::createDefault() {
    return RuntimeBuilder()
        .withConfig(RuntimeConfig{})
        .build();
}

Runtime RuntimeFactory::createTesting() {
    RuntimeConfig cfg;
    cfg.enableStatistics = true;
    cfg.enableMonitoring = true;
    cfg.autoRecovery = true;
    cfg.loaderSearchPaths = {"./TestLoaders"};

    return RuntimeBuilder()
        .withConfig(cfg)
        .build();
}

Runtime RuntimeFactory::createMinimal() {
    RuntimeConfig cfg;
    cfg.enableVendorRuntime = false;
    cfg.autoLoadPlugins = false;
    cfg.enableMonitoring = false;
    cfg.enableStatistics = false;

    auto logger = std::make_unique<NullLogger>();

    return RuntimeBuilder()
        .withConfig(cfg)
        .withLogger(std::move(logger))
        .build();
}

Runtime RuntimeFactory::createCLI() {
    RuntimeConfig cfg;
    cfg.logging.consoleOutput = true;
    cfg.logging.enabled = true;
    cfg.autoRecovery = true;
    cfg.maxRetries = 3;

    auto logger = std::make_unique<ConsoleLogger>();

    return RuntimeBuilder()
        .withConfig(cfg)
        .withLogger(std::move(logger))
        .build();
}

Runtime RuntimeFactory::createGUI() {
    RuntimeConfig cfg;
    cfg.logging.consoleOutput = true;
    cfg.logging.enabled = true;
    cfg.enableMonitoring = true;
    cfg.enableStatistics = true;
    cfg.autoRecovery = true;

    return RuntimeBuilder()
        .withConfig(cfg)
        .build();
}

Runtime RuntimeFactory::createEmbedded() {
    RuntimeConfig cfg;
    cfg.enableVendorRuntime = false;
    cfg.autoLoadPlugins = false;
    cfg.enableMonitoring = false;
    cfg.enableStatistics = false;
    cfg.logging.enabled = false;
    cfg.autoRecovery = false;
    cfg.maxRetries = 0;

    auto logger = std::make_unique<NullLogger>();

    return RuntimeBuilder()
        .withConfig(cfg)
        .withLogger(std::move(logger))
        .build();
}

} // namespace runtime
} // namespace mbootcore
