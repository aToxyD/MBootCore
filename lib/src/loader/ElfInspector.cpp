#include "mbootcore/loader/ElfInspector.hpp"

#include <cstring>
#include <sstream>

namespace mbootcore {

bool ElfInspector::hasValidMagic(const ByteBuffer& data) const noexcept {
    if (data.size() < 4) return false;
    return std::memcmp(data.data(), elfMagic.data(), 4) == 0;
}

bool ElfInspector::isElf(const ByteBuffer& data) const noexcept {
    return hasValidMagic(data);
}

bool ElfInspector::is32Bit(const ByteBuffer& data) const noexcept {
    return hasValidMagic(data) && data.size() >= 5 && data[4] == 1;
}

bool ElfInspector::is64Bit(const ByteBuffer& data) const noexcept {
    return hasValidMagic(data) && data.size() >= 5 && data[4] == 2;
}

uint64_t ElfInspector::entryPoint(const ByteBuffer& data) const noexcept {
    if (!hasValidMagic(data) || data.size() < 24) return 0;
    if (data[4] == 1) {
        if (data.size() < 28) return 0;
        uint64_t entry = 0;
        std::memcpy(&entry, data.data() + 24, 4);
        return entry;
    }
    if (data.size() < 32) return 0;
    uint64_t entry = 0;
    std::memcpy(&entry, data.data() + 24, 8);
    return entry;
}

std::string ElfInspector::architecture(const ByteBuffer& data) const {
    if (!hasValidMagic(data) || data.size() < 19) return "Unknown";
    uint16_t machine = 0;
    std::memcpy(&machine, data.data() + 18, 2);
    switch (machine) {
        case 0x00: return "No specific instruction set";
        case 0x02: return "SPARC";
        case 0x03: return "x86";
        case 0x08: return "MIPS";
        case 0x14: return "PowerPC";
        case 0x28: return "ARM";
        case 0x2A: return "SuperH";
        case 0x32: return "IA-64";
        case 0x3E: return "x86-64";
        case 0xB7: return "AArch64";
        case 0xF3: return "RISC-V";
        default: {
            std::ostringstream oss;
            oss << "Unknown (0x" << std::hex << machine << ")";
            return oss.str();
        }
    }
}

Result<ElfInspectionResult> ElfInspector::inspect(const ByteBuffer& data) {
    if (!hasValidMagic(data)) {
        return ErrorCode::InvalidElf;
    }

    if (data.size() < 7) {
        return ErrorCode::InvalidElf;
    }

    ElfInspectionResult result;
    result.header.is64Bit = (data[4] == 2);
    result.header.isLittleEndian = (data[5] == 1);
    result.header.version = data[6];
    result.architecture = architecture(data);

    if (result.header.is64Bit && data.size() >= 64) {
        std::memcpy(&result.header.type, data.data() + 16, 2);
        std::memcpy(&result.header.machine, data.data() + 18, 2);
        std::memcpy(&result.header.entry, data.data() + 24, 8);
        std::memcpy(&result.header.phoff, data.data() + 32, 8);
        std::memcpy(&result.header.shoff, data.data() + 40, 8);
        std::memcpy(&result.header.flags, data.data() + 48, 4);
        std::memcpy(&result.header.ehsize, data.data() + 52, 2);
        std::memcpy(&result.header.phentsize, data.data() + 54, 2);
        std::memcpy(&result.header.phnum, data.data() + 56, 2);
        std::memcpy(&result.header.shentsize, data.data() + 58, 2);
        std::memcpy(&result.header.shnum, data.data() + 60, 2);
        std::memcpy(&result.header.shstrndx, data.data() + 62, 2);
    } else if (!result.header.is64Bit && data.size() >= 52) {
        std::memcpy(&result.header.type, data.data() + 16, 2);
        std::memcpy(&result.header.machine, data.data() + 18, 2);
        std::memcpy(&result.header.entry, data.data() + 24, 4);
        std::memcpy(&result.header.phoff, data.data() + 28, 4);
        std::memcpy(&result.header.shoff, data.data() + 32, 4);
        std::memcpy(&result.header.flags, data.data() + 36, 4);
        std::memcpy(&result.header.ehsize, data.data() + 40, 2);
        std::memcpy(&result.header.phentsize, data.data() + 42, 2);
        std::memcpy(&result.header.phnum, data.data() + 44, 2);
        std::memcpy(&result.header.shentsize, data.data() + 46, 2);
        std::memcpy(&result.header.shnum, data.data() + 48, 2);
        std::memcpy(&result.header.shstrndx, data.data() + 50, 2);
    }

    result.hasProgramHeaders = result.header.phnum > 0 && result.header.phentsize > 0;
    result.hasSectionHeaders = result.header.shnum > 0 && result.header.shentsize > 0;

    return result;
}

std::vector<uint8_t> ElfInspector::computeHash(const ByteBuffer& data) const {
    if (data.empty()) return {};
    std::vector<uint8_t> hash(32, 0);
    size_t len = std::min(data.size(), size_t{256});
    for (size_t i = 0; i < len; ++i) {
        hash[i % 32] ^= data[i];
        hash[(i + 1) % 32] ^= static_cast<uint8_t>(data[i] << 3);
    }
    return hash;
}

} // namespace mbootcore
