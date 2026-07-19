#include <mbootcore/MBootCore.hpp>
#include <mbootcore/runtime/RuntimeFactory.hpp>
#include <mbootcore/runtime/Runtime.hpp>
#include <mbootcore/logging/ConsoleLogger.hpp>
#include <sdk/Version.hpp>
#include <sdk/SDKInfo.hpp>
#include <iostream>
#include <cstdlib>
#include <string>
#include <vector>
#include <sstream>
#include <algorithm>

using namespace mbootcore;

struct CliOptions {
    bool help{false};
    bool version{false};
    bool discover{false};
    bool info{false};
    bool diagnose{false};
    std::string device;
    std::string firmwarePath;
    std::string partition;
    std::string action;
    int timeoutMs{5000};
};

static void printHelp(const char* prog) {
    std::cout << "MBootCore CLI Embedding Example\n"
              << "Usage: " << prog << " [options]\n\n"
              << "Options:\n"
              << "  -h, --help              Show this help\n"
              << "  -V, --version           Print SDK version\n"
              << "  -d, --discover          Discover devices\n"
              << "  -i, --info              Show SDK info\n"
              << "  --diagnose              Run SDK diagnostics\n"
              << "  --device <name>         Target device name\n"
              << "  --firmware <path>       Firmware package path\n"
              << "  --partition <name>      Partition name\n"
              << "  --action <action>       Action: flash|backup|restore|erase\n"
              << "  --timeout <ms>          Discovery timeout in ms\n"
              << std::endl;
}

static void printSDKVersion() {
    auto ver = sdk::getSDKVersion();
    auto info = sdk::getVersionInfo();
    std::cout << "MBootCore SDK " << ver.toString() << "\n"
              << "Core:    " << info.coreVersion.toString() << "\n"
              << "Build:   " << info.buildInfo.buildDate << " "
              << info.buildInfo.buildTime << "\n"
              << "Git:     " << info.gitInfo.commitShortHash
              << (info.gitInfo.dirty ? " (dirty)" : "") << "\n"
              << "Branch:  " << info.gitInfo.branch << "\n"
              << "Compiler: " << info.compilerInfo.compilerName << " "
              << info.compilerInfo.compilerVersion << "\n"
              << "Platform: " << info.platformInfo.osName << " "
              << info.platformInfo.osVersion << "\n"

              << std::endl;
}

static void printSDKInfo() {
    auto info = sdk::SDKInfo::collect();
    std::cout << info.toString() << std::endl;
}

static void runDiagnostics() {
    sdk::SDKDoctor doctor;
    auto report = doctor.runAllChecks();
    std::cout << doctor.generateReport(report) << std::endl;
}

static bool parseArgs(int argc, char* argv[], CliOptions& opts) {
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-h" || arg == "--help") {
            opts.help = true;
        } else if (arg == "-V" || arg == "--version") {
            opts.version = true;
        } else if (arg == "-d" || arg == "--discover") {
            opts.discover = true;
        } else if (arg == "-i" || arg == "--info") {
            opts.info = true;
        } else if (arg == "--diagnose") {
            opts.diagnose = true;
        } else if (arg == "--device" && i + 1 < argc) {
            opts.device = argv[++i];
        } else if (arg == "--firmware" && i + 1 < argc) {
            opts.firmwarePath = argv[++i];
        } else if (arg == "--partition" && i + 1 < argc) {
            opts.partition = argv[++i];
        } else if (arg == "--action" && i + 1 < argc) {
            opts.action = argv[++i];
        } else if (arg == "--timeout" && i + 1 < argc) {
            opts.timeoutMs = std::atoi(argv[++i]);
            if (opts.timeoutMs <= 0) opts.timeoutMs = 5000;
        } else {
            std::cerr << "Unknown option: " << arg << "\n";
            return false;
        }
    }
    return true;
}

static int runAction(const CliOptions& opts) {
    auto runtime = runtime::RuntimeFactory::createCLI();
    auto result = runtime.initialize();
    if (!result.isOk()) {
        std::cerr << "Runtime init failed\n";
        return EXIT_FAILURE;
    }

    if (opts.action == "flash") {
        if (opts.firmwarePath.empty()) {
            std::cerr << "--firmware required for flash action\n";
            runtime.shutdown();
            return EXIT_FAILURE;
        }
        auto flashResult = runtime.flash(opts.firmwarePath);
        if (flashResult.isOk()) {
            std::cout << "Flash completed successfully\n";
        } else {
            std::cerr << "Flash failed: " << toString(flashResult.error()) << "\n";
            runtime.shutdown();
            return EXIT_FAILURE;
        }
    } else if (opts.action == "backup") {
        if (opts.partition.empty()) {
            std::cerr << "--partition required for backup action\n";
            runtime.shutdown();
            return EXIT_FAILURE;
        }
        auto data = runtime.backup(opts.partition);
        if (data.isOk()) {
            std::cout << "Backup of '" << opts.partition << "': "
                      << data.value().size() << " bytes\n";
        } else {
            std::cerr << "Backup failed\n";
            runtime.shutdown();
            return EXIT_FAILURE;
        }
    } else if (opts.action == "erase") {
        if (opts.partition.empty()) {
            std::cerr << "--partition required for erase action\n";
            runtime.shutdown();
            return EXIT_FAILURE;
        }
        auto eraseResult = runtime.erasePartition(opts.partition);
        if (eraseResult.isOk()) {
            std::cout << "Erased partition: " << opts.partition << "\n";
        } else {
            std::cerr << "Erase failed\n";
            runtime.shutdown();
            return EXIT_FAILURE;
        }
    } else {
        std::cerr << "Unknown action: " << opts.action << "\n";
        runtime.shutdown();
        return EXIT_FAILURE;
    }

    runtime.shutdown();
    return EXIT_SUCCESS;
}

int main(int argc, char* argv[]) {
    CliOptions opts;

    if (!parseArgs(argc, argv, opts)) {
        printHelp(argv[0]);
        return EXIT_FAILURE;
    }

    if (opts.help) {
        printHelp(argv[0]);
        return EXIT_SUCCESS;
    }

    if (opts.version) {
        printSDKVersion();
        return EXIT_SUCCESS;
    }

    if (opts.info) {
        printSDKInfo();
        return EXIT_SUCCESS;
    }

    if (opts.diagnose) {
        runDiagnostics();
        return EXIT_SUCCESS;
    }

    if (opts.discover) {
        auto runtime = runtime::RuntimeFactory::createCLI();
        auto result = runtime.initialize();
        if (!result.isOk()) {
            std::cerr << "Runtime init failed\n";
            return EXIT_FAILURE;
        }

        auto devices = runtime.discover(
            std::chrono::milliseconds(opts.timeoutMs));
        if (devices.isOk() && !devices.value().empty()) {
            std::cout << "Discovered " << devices.value().size()
                      << " device(s):\n";
            for (const auto& d : devices.value()) {
                std::cout << "  - " << d.friendlyName
                          << " (vendor="
                          << std::to_string(static_cast<int>(d.vendor))
                          << ", transport="
                          << std::to_string(static_cast<int>(d.transport))
                          << ")\n";
            }
        } else {
            std::cout << "No devices found\n";
        }

        runtime.shutdown();
        return EXIT_SUCCESS;
    }

    if (!opts.action.empty()) {
        return runAction(opts);
    }

    // Default: show help
    printHelp(argv[0]);
    return EXIT_SUCCESS;
}
