#pragma once

#include "mbootcore/domain/ILoader.hpp"
#include "mbootcore/domain/ILogger.hpp"

#include <memory>
#include <vector>
#include <string>

namespace mbootcore {

class LoaderManager {
public:
    explicit LoaderManager(ILogger& logger);

    Result<std::unique_ptr<ILoader>> findLoader(const DeviceId& deviceId);

    Result<std::unique_ptr<ILoader>> loadFromFile(const std::string& path);
    Result<std::unique_ptr<ILoader>> loadFromBuffer(ByteBuffer data);

    Result<bool> isLoaderCompatible(const ILoader& loader,
                                    const DeviceId& deviceId);

    std::vector<LoaderInfo> availableLoaders() const;

    void addSearchPath(const std::string& path);

private:
    ILogger& m_logger;
    std::vector<std::string> m_searchPaths;
};

} // namespace mbootcore
