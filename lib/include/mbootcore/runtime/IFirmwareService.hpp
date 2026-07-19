#pragma once

#include <mbootcore/domain/Error.hpp>
#include <mbootcore/domain/Types.hpp>
#include <mbootcore/firmware/FirmwarePackage.hpp>
#include <mbootcore/firmware/FirmwareTypes.hpp>

#include <memory>
#include <string>

namespace mbootcore {
namespace runtime {

class IFirmwareService {
public:
    virtual ~IFirmwareService() = default;

    virtual Result<void> flash(const std::string& packagePath) = 0;

    virtual Result<void> flash(firmware::FirmwarePackage& package) = 0;

    virtual Result<ByteBuffer> read(uint64_t address, size_t size) = 0;

    virtual Result<void> write(uint64_t address, const ByteBuffer& data) = 0;

    virtual Result<void> erase(uint64_t address, size_t size) = 0;

    virtual Result<void> verify(uint64_t address, const ByteBuffer& expected) = 0;

    virtual Result<ByteBuffer> readPartition(const std::string& name) = 0;

    virtual Result<void> writePartition(const std::string& name, const ByteBuffer& data) = 0;

    virtual Result<void> erasePartition(const std::string& name) = 0;

    virtual Result<ByteBuffer> backup(const std::string& partition) = 0;

    virtual Result<void> restore(const std::string& partition, const ByteBuffer& data) = 0;

    virtual Result<std::unique_ptr<firmware::FirmwarePackage>> loadFirmwarePackage(
        const std::string& path) = 0;

    virtual firmware::FirmwarePackage* loadedPackage() const noexcept = 0;
};

} // namespace runtime
} // namespace mbootcore
