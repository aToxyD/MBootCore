#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace mbootcore {

struct PartitionEntry {
    std::string name;
    std::string guid;
    uint64_t startSector{0};
    uint64_t length{0};
    uint64_t byteOffset{0};
    uint64_t byteLength{0};
    std::string filename;
    std::string label;
    uint32_t attributes{0};
    uint32_t flags{0};
    bool isReadOnly{false};
    bool isBootable{false};
    bool isHidden{false};
    bool isSystem{false};
};

struct PartitionTable {
    std::string type;
    uint32_t entryCount{0};
    uint32_t entrySize{0};
    uint64_t firstUsableLba{0};
    uint64_t lastUsableLba{0};
    std::vector<PartitionEntry> entries;
};

} // namespace mbootcore
