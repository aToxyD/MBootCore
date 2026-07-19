#pragma once

#include <mbootcore/domain/Error.hpp>
#include <mbootcore/plugin/IPlugin.hpp>
#include <mbootcore/plugin/PluginTypes.hpp>
#include <mbootcore/vendor/IVendor.hpp>

#include <memory>
#include <string>
#include <vector>

namespace mbootcore {
namespace runtime {

class IPluginService {
public:
    virtual ~IPluginService() = default;

    virtual Result<void> installPlugin(
        std::unique_ptr<plugin::IPlugin> plugin) = 0;

    virtual Result<void> removePlugin(const std::string& name) = 0;

    virtual std::vector<std::string> listPlugins() const = 0;

    virtual Result<void> registerVendor(
        std::unique_ptr<vendor::IVendor> vendor) = 0;
};

} // namespace runtime
} // namespace mbootcore
