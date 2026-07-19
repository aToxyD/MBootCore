#include "mbootcore/discovery/DeviceDiscoveryEngine.hpp"

#include <algorithm>

namespace mbootcore {
namespace discovery {

DeviceDiscoveryEngine::DeviceDiscoveryEngine(ProtocolRegistry& registry)
    : m_registry(&registry) {}

DiscoveryResult DeviceDiscoveryEngine::discoverAll(std::chrono::milliseconds timeout) {
    DiscoveryResult result;
    auto start = std::chrono::steady_clock::now();

    for (const auto& detector : m_registry->detectors()) {
        auto deviceResult = detector->enumerate();
        if (deviceResult.isOk()) {
            for (auto& dev : deviceResult.value()) {
                auto probeResult = probeDevice(dev);
                if (probeResult.isOk()) {
                    auto& updatedDev = probeResult.value();
                    dev.vendor = updatedDev.vendor;
                    dev.bootMode = updatedDev.bootMode;
                    dev.protocolHint = updatedDev.protocolHint;
                    dev.friendlyName = updatedDev.friendlyName;
                }
                result.devices.push_back(std::move(dev));
            }
        }

        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - start);
        if (elapsed >= timeout) {
            result.timedOut = true;
            break;
        }
    }

    if (result.devices.empty()) {
        result.error = true;
    }

    result.elapsedMs = static_cast<uint32_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - start).count());
    return result;
}

std::vector<DeviceDescriptor> DeviceDiscoveryEngine::discoverByVendor(Vendor vendor,
    std::chrono::milliseconds timeout) {
    std::vector<DeviceDescriptor> matches;
    auto all = discoverAll(timeout);
    for (auto& dev : all.devices) {
        if (dev.vendor == vendor) {
            matches.push_back(std::move(dev));
        }
    }
    return matches;
}

DiscoveryResult DeviceDiscoveryEngine::discoverAndIdentify(std::chrono::milliseconds timeout) {
    auto result = discoverAll(timeout);
    for (auto& dev : result.devices) {
        for (const auto& detector : m_registry->detectors()) {
            auto identified = detector->identify(dev);
            if (identified.isOk()) {
                dev = std::move(identified.value());
            }
        }
    }
    return result;
}

Result<DeviceDescriptor> DeviceDiscoveryEngine::probeDevice(const DeviceDescriptor& hint) {
    DeviceDescriptor result = hint;

    for (const auto& detector : m_registry->detectors()) {
        auto probeResult = detector->probe(result);
        if (probeResult.isOk()) {
            auto identified = detector->identify(result);
            if (identified.isOk()) {
                result = std::move(identified.value());
            }
            return result;
        }
    }

    if (hint.isValid()) {
        return result;
    }
    return ErrorCode::DeviceNotFound;
}

} // namespace discovery
} // namespace mbootcore
