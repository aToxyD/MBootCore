#pragma once

#include <mbootcore/plugin/IPlugin.hpp>

#include <vector>

namespace mbootcore {
namespace plugin {

class IProtocolPlugin : public virtual IPlugin {
public:
    ~IProtocolPlugin() override = default;

    virtual discovery::ProtocolType protocolType() const noexcept = 0;

    virtual std::vector<discovery::ProtocolType> supportedProtocols() const noexcept = 0;
};

} // namespace plugin
} // namespace mbootcore
