#pragma once

#include "mbootcore/gpt/GPTModels.hpp"
#include "mbootcore/domain/Types.hpp"
#include "mbootcore/domain/Error.hpp"
#include "mbootcore/generic/IFlashDevice.hpp"

namespace mbootcore {
namespace gpt {

struct WriteResult {
    bool primaryWritten{false};
    bool backupWritten{false};
    bool entriesWritten{false};
    bool headerCRCUpdated{false};
    bool entryCRCUpdated{false};
    std::vector<ErrorCode> warnings;
};

class GPTWriter {
public:
    explicit GPTWriter(IFlashDevice& device);

    Result<void> writePrimaryHeader(const GPTHeader& header);
    Result<void> writeBackupHeader(const GPTHeader& header);
    Result<void> writePartitionEntries(const GPTHeader& header,
                                        const std::vector<PartitionEntry>& entries);
    Result<void> writePrimaryAndBackup(const GPTHeader& header,
                                        const std::vector<PartitionEntry>& entries);

    Result<void> updateHeaderCRC(GPTHeader& header);
    Result<uint32_t> computeHeaderCRC(const GPTHeader& header) const;
    Result<uint32_t> computeEntryCRC(const std::vector<PartitionEntry>& entries,
                                      uint64_t count, uint64_t entrySize) const;

    GPTLayout layout() const noexcept;

private:
    ByteBuffer serializeHeader(const GPTHeader& header) const;
    ByteBuffer serializeEntries(const std::vector<PartitionEntry>& entries,
                                 uint64_t count, uint64_t entrySize) const;
    Result<void> writeSector(uint64_t lba, const ByteBuffer& data);
    Result<void> writeSectors(uint64_t lba, const ByteBuffer& data);
    uint64_t lbaToByte(uint64_t lba) const noexcept;
    uint32_t computeCRC32(const ByteBuffer& data) const;

    IFlashDevice& m_device;
    GPTLayout m_layout{};
};

}} // namespace mbootcore::gpt
