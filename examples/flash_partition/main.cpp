#include <mbootcore/MBootCore.hpp>
#include <mbootcore/runtime/RuntimeFactory.hpp>
#include <mbootcore/runtime/Runtime.hpp>
#include <mbootcore/runtime/RuntimeCallbacks.hpp>
#include <mbootcore/logging/ConsoleLogger.hpp>
#include <iostream>
#include <cstdlib>

int main(int argc, char* argv[]) {
    using namespace mbootcore;

    if (argc < 2) {
        std::cerr << "Usage: example_flash_partition <firmware_path>\n";
        return EXIT_FAILURE;
    }

    auto logger = std::make_shared<ConsoleLogger>();

    auto runtime = runtime::RuntimeFactory::createDefault();
    auto result = runtime.initialize();
    if (!result.isOk()) {
        std::cerr << "Init failed: " << toString(result.error()) << std::endl;
        return EXIT_FAILURE;
    }

    logger->info("example", "Loading firmware from: " + std::string(argv[1]));

    runtime::RuntimeCallbacks cbs;
    cbs.onLog = [&logger](const std::string& msg) {
        logger->info("log", msg);
    };
    cbs.onProgress = [&logger](const std::string& op, double pct) {
        logger->info("progress", op + ": " + std::to_string(static_cast<int>(pct * 100)) + "%");
    };
    runtime.setCallbacks(std::move(cbs));

    auto flashResult = runtime.flash(argv[1]);
    if (flashResult.isOk()) {
        logger->info("example", "Flash completed successfully");
    } else {
        logger->error("example", "Flash failed: " + std::string(toString(flashResult.error())));
    }

    runtime.shutdown();
    return flashResult.isOk() ? EXIT_SUCCESS : EXIT_FAILURE;
}
