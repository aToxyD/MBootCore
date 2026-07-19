#include "mbootcore/loader/LoaderManager.hpp"
#include "mbootcore/loader/ProgrammerLoader.hpp"
#include <fstream>
#include <vector>
#include <algorithm>
#include <iterator>

namespace mbootcore {

LoaderManager::LoaderManager(ILogger& logger)
    : m_logger(logger) {}

Result<std::unique_ptr<ILoader>> LoaderManager::findLoader(const DeviceId& deviceId) {
    m_logger.info("Loader", "Searching for compatible loader...");
    for (const auto& dir : m_searchPaths) {
        auto result = loadFromFile(dir);
        if (result.isOk()) {
            auto& loader = result.value();
            auto compat = loader->isCompatibleWith(deviceId);
            if (compat.isOk() && compat.value()) {
                auto parseResult = loader->parse();
                if (parseResult.isOk()) {
                    return result;
                }
            }
        }
    }
    return ErrorCode::LoaderNotFound;
}

Result<std::unique_ptr<ILoader>> LoaderManager::loadFromFile(const std::string& path) {
    m_logger.info("Loader", "Loading from file: " + path);
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file) {
        return ErrorCode::LoaderNotFound;
    }
    auto size = file.tellg();
    file.seekg(0, std::ios::beg);
    ByteBuffer buffer(static_cast<size_t>(size));
    if (!file.read(reinterpret_cast<char*>(buffer.data()), size)) {
        return ErrorCode::LoaderNotFound;
    }
    auto loader = std::make_unique<ProgrammerLoader>(std::move(buffer));
    auto parseResult = loader->parse();
    if (parseResult.isError()) {
        return parseResult.error();
    }
    return Result<std::unique_ptr<ILoader>>(std::move(loader));
}

Result<std::unique_ptr<ILoader>> LoaderManager::loadFromBuffer(ByteBuffer data) {
    m_logger.info("Loader", "Loading from buffer");
    auto loader = std::make_unique<ProgrammerLoader>(std::move(data));
    auto parseResult = loader->parse();
    if (parseResult.isError()) {
        return parseResult.error();
    }
    return Result<std::unique_ptr<ILoader>>(std::move(loader));
}

Result<bool> LoaderManager::isLoaderCompatible(const ILoader& loader,
                                                const DeviceId& deviceId) {
    return loader.isCompatibleWith(deviceId);
}

std::vector<LoaderInfo> LoaderManager::availableLoaders() const {
    return {};
}

void LoaderManager::addSearchPath(const std::string& path) {
    m_searchPaths.push_back(path);
}

} // namespace mbootcore
