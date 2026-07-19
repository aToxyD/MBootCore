#include <mbootcore/MBootCore.hpp>
#include <mbootcore/runtime/RuntimeFactory.hpp>
#include <mbootcore/runtime/Runtime.hpp>
#include <mbootcore/generic/ProgressInfo.hpp>
#include <mbootcore/logging/ConsoleLogger.hpp>
#include <sdk/Version.hpp>
#include <fstream>
#include <iostream>
#include <cstdlib>
#include <vector>
#include <string>
#include <filesystem>

namespace fs = std::filesystem;

static bool writeFile(const std::string& path, const mbootcore::ByteBuffer& data) {
    std::ofstream ofs(path, std::ios::binary);
    if (!ofs) {
        std::cerr << "Cannot open " << path << " for writing\n";
        return false;
    }
    ofs.write(reinterpret_cast<const char*>(data.data()),
              static_cast<std::streamsize>(data.size()));
    return ofs.good();
}

static mbootcore::ByteBuffer readFile(const std::string& path) {
    std::ifstream ifs(path, std::ios::binary | std::ios::ate);
    if (!ifs) return {};
    auto size = static_cast<std::size_t>(ifs.tellg());
    ifs.seekg(0);
    mbootcore::ByteBuffer buf(size);
    ifs.read(reinterpret_cast<char*>(buf.data()), static_cast<std::streamsize>(size));
    return buf;
}

int main(int argc, char* argv[]) {
    using namespace mbootcore;

    auto logger = std::make_shared<ConsoleLogger>();

    if (argc < 2) {
        std::cerr << "Usage: example_backup_partition <partition_name> [restore_file]\n";
        std::cerr << "  If restore_file is given, restores that file to the partition.\n";
        std::cerr << "  Otherwise, backs up the partition to disk.\n";
        return EXIT_FAILURE;
    }

    std::string partition = argv[1];
    bool restoreMode = (argc >= 3);

    auto runtime = runtime::RuntimeFactory::createDefault();
    auto result = runtime.initialize();
    if (!result.isOk()) {
        std::cerr << "Init failed: " << toString(result.error()) << std::endl;
        return EXIT_FAILURE;
    }

    logger->info("example",
                 "MBootCore Runtime v" + sdk::getSDKVersion().toString());

    // Discover and connect
    auto devices = runtime.discover(std::chrono::seconds(5));
    if (!devices.isOk() || devices.value().empty()) {
        logger->error("example", "No devices found");
        runtime.shutdown();
        return EXIT_FAILURE;
    }

    auto& dev = devices.value().front();
    logger->info("example", "Connecting to: " + dev.friendlyName);

    auto connResult = runtime.connect(dev);
    if (!connResult.isOk()) {
        logger->error("example",
                      "Connection failed: " + std::string(toString(connResult.error())));
        runtime.shutdown();
        return EXIT_FAILURE;
    }
    logger->info("example", "Connected successfully");

    if (restoreMode) {
        // Restore partition from file
        std::string restorePath = argv[2];
        logger->info("example",
                     "Restoring partition '" + partition + "' from " + restorePath);

        auto data = readFile(restorePath);
        if (data.empty()) {
            logger->error("example", "Failed to read restore file: " + restorePath);
            runtime.disconnect();
            runtime.shutdown();
            return EXIT_FAILURE;
        }
        logger->info("example",
                     "Read " + std::to_string(data.size()) + " bytes from file");

        auto backupResult = runtime.backup(partition);
        if (backupResult.isOk()) {
            logger->info("example",
                         "Pre-restore backup saved (" +
                             std::to_string(backupResult.value().size()) + " bytes)");
        }

        auto restoreResult = runtime.restore(partition, data);
        if (restoreResult.isOk()) {
            logger->info("example",
                         "Partition '" + partition + "' restored successfully (" +
                             std::to_string(data.size()) + " bytes)");
        } else {
            logger->error("example",
                          "Restore failed: " + std::string(toString(restoreResult.error())));
        }
    } else {
        // Backup partition to file
        logger->info("example", "Reading partition: " + partition);
        auto readResult = runtime.readPartition(partition);
        if (!readResult.isOk()) {
            logger->error("example",
                          "Read failed: " + std::string(toString(readResult.error())));
            runtime.disconnect();
            runtime.shutdown();
            return EXIT_FAILURE;
        }

        auto& data = readResult.value();
        logger->info("example",
                     "Read " + std::to_string(data.size()) + " bytes from partition '" +
                         partition + "'");

        auto backupPath = partition + "_backup.bin";
        if (writeFile(backupPath, data)) {
            logger->info("example",
                         "Backup saved to " + backupPath + " (" +
                             std::to_string(data.size()) + " bytes)");

            auto fileSize = fs::file_size(backupPath);
            logger->info("example",
                         "Verification: file on disk is " + std::to_string(fileSize) + " bytes");
        } else {
            logger->error("example", "Failed to write backup file");
        }
    }

    runtime.disconnect();
    runtime.shutdown();
    return EXIT_SUCCESS;
}
