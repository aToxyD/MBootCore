#pragma once

#include "mbootcore/gpt/GPTModels.hpp"
#include "mbootcore/domain/Types.hpp"
#include "mbootcore/domain/Error.hpp"
#include "mbootcore/generic/IFlashDevice.hpp"

namespace mbootcore {
namespace gpt {

struct ParserResult {
    GPTTable table;
    std::vector<ErrorCode> warnings;
    std::vector<ErrorCode> errors;
};

class GPTParser {
public:
    explicit GPTParser(IFlashDevice& device);

    ParserResult parse();

    ParserResult parsePrimary();
    ParserResult parseBackup();

    bool hasValidProtectiveMBR() const noexcept;
    const GPTLayout& layout() const noexcept;

private:
    ProtectiveMBREntry readProtectiveMBR(const ByteBuffer& sector);
    GPTHeader readHeader(const ByteBuffer& data, uint64_t lba);
    std::vector<PartitionEntry> readEntries(const ByteBuffer& data,
                                             uint64_t count, uint64_t entrySize);
    ByteBuffer readSector(uint64_t lba);
    ByteBuffer readSectors(uint64_t lba, uint64_t count);
    uint32_t computeCRC32(const ByteBuffer& data) const;
    uint32_t computeEntryCRC32(const std::vector<PartitionEntry>& entries,
                                uint64_t count, uint64_t entrySize) const;
    uint64_t lbaToByte(uint64_t lba) const noexcept;
    uint64_t byteToLBA(uint64_t byteAddr) const noexcept;

    IFlashDevice& m_device;
    GPTLayout m_layout{};
    bool m_hasValidMBR{false};
};

}} // namespace mbootcore::gpt
