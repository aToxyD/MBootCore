#include "mbootcore/discovery/ProtocolRegistry.hpp"

#include <algorithm>

namespace mbootcore {
namespace discovery {

void ProtocolRegistry::registerDetector(std::unique_ptr<IDeviceDetector> detector) {
    if (detector) {
        m_detectors.push_back(std::move(detector));
    }
}

void ProtocolRegistry::registerNegotiator(std::unique_ptr<IProtocolNegotiator> negotiator) {
    if (negotiator) {
        m_negotiators.push_back(std::move(negotiator));
    }
}

void ProtocolRegistry::registerFactory(std::unique_ptr<IProtocolFactory> factory) {
    if (factory) {
        m_factories[factory->protocolType()] = std::move(factory);
    }
}

void ProtocolRegistry::unregisterDetector(const std::string& name) {
    auto it = std::remove_if(m_detectors.begin(), m_detectors.end(),
        [&](const auto& d) { return d->name() == name; });
    m_detectors.erase(it, m_detectors.end());
}

void ProtocolRegistry::unregisterNegotiator(const std::string& name) {
    auto it = std::remove_if(m_negotiators.begin(), m_negotiators.end(),
        [&](const auto& n) { return n->name() == name; });
    m_negotiators.erase(it, m_negotiators.end());
}

void ProtocolRegistry::unregisterFactory(ProtocolType type) {
    m_factories.erase(type);
}

IDeviceDetector* ProtocolRegistry::findDetector(const std::string& name) const {
    for (const auto& d : m_detectors) {
        if (d->name() == name) return d.get();
    }
    return nullptr;
}

IProtocolNegotiator* ProtocolRegistry::findNegotiator(const std::string& name) const {
    for (const auto& n : m_negotiators) {
        if (n->name() == name) return n.get();
    }
    return nullptr;
}

IProtocolFactory* ProtocolRegistry::findFactory(ProtocolType type) const {
    auto it = m_factories.find(type);
    return it != m_factories.end() ? it->second.get() : nullptr;
}

void ProtocolRegistry::clear() {
    m_detectors.clear();
    m_negotiators.clear();
    m_factories.clear();
}

} // namespace discovery
} // namespace mbootcore
