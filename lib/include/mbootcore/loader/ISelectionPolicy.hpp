#pragma once

#include "mbootcore/loader/LoaderMetadata.hpp"

#include <string>
#include <vector>

namespace mbootcore {

enum class SelectionPriority : uint8_t {
    ExactMatch     = 100,
    PkHashMatch    = 80,
    ChipsetMatch   = 60,
    VendorMatch    = 40,
    ProtocolMatch  = 20,
    Default        = 0
};

class ISelectionPolicy {
public:
    virtual ~ISelectionPolicy() = default;

    struct Candidate {
        std::string name;
        LoaderMetadata metadata;
        SelectionPriority priority{SelectionPriority::Default};
    };

    virtual std::string select(const std::vector<Candidate>& candidates) = 0;
};

} // namespace mbootcore
