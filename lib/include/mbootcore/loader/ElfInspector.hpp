#pragma once

#include "mbootcore/loader/IELFInspector.hpp"

#include <array>
#include <cstring>

namespace mbootcore {

class ElfInspector : public IElfInspector {
public:
    Result<ElfInspectionResult> inspect(const ByteBuffer& data) override;
    bool isElf(const ByteBuffer& data) const noexcept override;
    bool is32Bit(const ByteBuffer& data) const noexcept override;
    bool is64Bit(const ByteBuffer& data) const noexcept override;
    uint64_t entryPoint(const ByteBuffer& data) const noexcept override;
    std::string architecture(const ByteBuffer& data) const override;
    std::vector<uint8_t> computeHash(const ByteBuffer& data) const override;

private:
    static constexpr std::array<uint8_t, 4> elfMagic{0x7F, 'E', 'L', 'F'};

    bool hasValidMagic(const ByteBuffer& data) const noexcept;
};

} // namespace mbootcore
