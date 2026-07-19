#pragma once

#include "mbootcore/loader/ISelectionPolicy.hpp"

#include <algorithm>

namespace mbootcore {

class PrioritySelection : public ISelectionPolicy {
public:
    std::string select(const std::vector<Candidate>& candidates) override {
        if (candidates.empty()) return {};
        return std::max_element(
            candidates.begin(), candidates.end(),
            [](const Candidate& a, const Candidate& b) {
                return static_cast<uint8_t>(a.priority) < static_cast<uint8_t>(b.priority);
            })->name;
    }
};

} // namespace mbootcore
