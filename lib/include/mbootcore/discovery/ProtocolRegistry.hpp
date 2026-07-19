#pragma once

#include <mbootcore/discovery/IDeviceDetector.hpp>
#include <mbootcore/discovery/IProtocolNegotiator.hpp>
#include <mbootcore/discovery/IProtocolFactory.hpp>

#include <memory>
#include <vector>
#include <string>
#include <unordered_map>

namespace mbootcore {
namespace discovery {

class ProtocolRegistry {
public:
    ProtocolRegistry() = default;
    ~ProtocolRegistry() = default;

    ProtocolRegistry(const ProtocolRegistry&) = delete;
    ProtocolRegistry& operator=(const ProtocolRegistry&) = delete;
    ProtocolRegistry(ProtocolRegistry&&) = delete;
    ProtocolRegistry& operator=(ProtocolRegistry&&) = delete;

    void registerDetector(std::unique_ptr<IDeviceDetector> detector);
    void registerNegotiator(std::unique_ptr<IProtocolNegotiator> negotiator);
    void registerFactory(std::unique_ptr<IProtocolFactory> factory);

    void unregisterDetector(const std::string& name);
    void unregisterNegotiator(const std::string& name);
    void unregisterFactory(ProtocolType type);

    IDeviceDetector* findDetector(const std::string& name) const;
    IProtocolNegotiator* findNegotiator(const std::string& name) const;
    IProtocolFactory* findFactory(ProtocolType type) const;

    const std::vector<std::unique_ptr<IDeviceDetector>>& detectors() const { return m_detectors; }
    const std::vector<std::unique_ptr<IProtocolNegotiator>>& negotiators() const { return m_negotiators; }
    const std::unordered_map<ProtocolType, std::unique_ptr<IProtocolFactory>>& factories() const { return m_factories; }

    void clear();

    std::size_t detectorCount() const { return m_detectors.size(); }
    std::size_t negotiatorCount() const { return m_negotiators.size(); }
    std::size_t factoryCount() const { return m_factories.size(); }

private:
    std::vector<std::unique_ptr<IDeviceDetector>> m_detectors;
    std::vector<std::unique_ptr<IProtocolNegotiator>> m_negotiators;
    std::unordered_map<ProtocolType, std::unique_ptr<IProtocolFactory>> m_factories;
};

} // namespace discovery
} // namespace mbootcore
