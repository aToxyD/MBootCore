#include "mbootcore/discovery/ProtocolNegotiationEngine.hpp"

#include <algorithm>

namespace mbootcore {
namespace discovery {

ProtocolNegotiationEngine::ProtocolNegotiationEngine(ProtocolRegistry& registry)
    : m_registry(&registry) {}

NegotiationResult ProtocolNegotiationEngine::negotiate(const DeviceDescriptor& descriptor) {
    auto all = negotiateAll(descriptor);
    return bestMatch(all);
}

std::vector<NegotiationResult> ProtocolNegotiationEngine::negotiateAll(const DeviceDescriptor& descriptor) {
    std::vector<NegotiationResult> results;

    for (const auto& negotiator : m_registry->negotiators()) {
        auto result = negotiator->negotiate(descriptor);
        if (result.protocol != ProtocolType::Unknown && result.confidence > 0) {
            result.descriptor = descriptor;
            results.push_back(std::move(result));
        }
    }

    std::sort(results.begin(), results.end(),
        [](const NegotiationResult& a, const NegotiationResult& b) {
            return a.confidence > b.confidence;
        });

    return results;
}

NegotiationResult ProtocolNegotiationEngine::bestMatch(const std::vector<NegotiationResult>& results) {
    if (results.empty()) {
        NegotiationResult fallback;
        fallback.protocol = ProtocolType::Unknown;
        fallback.confidence = 0;
        fallback.reason = "No negotiator matched";
        return fallback;
    }
    return results.front();
}

} // namespace discovery
} // namespace mbootcore
