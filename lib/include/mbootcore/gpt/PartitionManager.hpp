#pragma once

#include "mbootcore/gpt/GPTModels.hpp"
#include "mbootcore/gpt/GPTParser.hpp"
#include "mbootcore/gpt/GPTWriter.hpp"
#include "mbootcore/domain/Types.hpp"
#include "mbootcore/domain/Error.hpp"
#include "mbootcore/generic/IFlashDevice.hpp"
#include "mbootcore/generic/ProgressInfo.hpp"

#include <string>
#include <vector>
#include <functional>
#include <optional>

namespace mbootcore {
namespace gpt {

class PartitionManager {
public:
    explicit PartitionManager(IFlashDevice& device);

    Result<void> open();
    Result<GPTTable> refreshTable();

    // Discovery
    Result<std::vector<PartitionInfo>> listPartitions();
    Result<PartitionInfo> findByName(const std::string& name);
    Result<PartitionInfo> findByGUID(const Guid& guid);
    Result<std::vector<PartitionInfo>> findByType(const Guid& typeGUID);
    bool exists(const std::string& name);
    size_t partitionCount() const noexcept;

    // Partition operations (byte-level using GPT layout)
    Result<ByteBuffer> readPartition(const std::string& name);
    Result<void> writePartition(const std::string& name, const ByteBuffer& data);
    Result<void> erasePartition(const std::string& name);
    Result<void> trimPartition(const std::string& name);

    // Backup / Restore
    Result<ByteBuffer> backupPartition(const std::string& name);
    Result<void> restorePartition(const std::string& name, const ByteBuffer& data);

    // Verification
    Result<bool> verifyPartition(const std::string& name, const ByteBuffer& expected);
    Result<bool> comparePartition(const std::string& name,
                                   const ByteBuffer& data);

    // Recovery
    Result<void> recoverFromBackup();

    // Accessors
    const GPTTable& table() const noexcept;
    const GPTLayout& layout() const noexcept;
    bool isOpen() const noexcept;

    void setProgressCallback(ProgressCallback callback);

private:
    Result<PartitionInfo> entryToInfo(const PartitionEntry& entry) const;

    IFlashDevice& m_device;
    GPTParser m_parser;
    GPTWriter m_writer;
    GPTTable m_table;
    GPTLayout m_layout;
    bool m_open{false};
    ProgressCallback m_callback;
};

}} // namespace mbootcore::gpt
