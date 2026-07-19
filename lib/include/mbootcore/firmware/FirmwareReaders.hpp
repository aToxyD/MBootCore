#pragma once

#include <mbootcore/firmware/IFirmwareReader.hpp>
#include <mbootcore/firmware/FirmwarePackage.hpp>
#include <string>
#include <memory>
#include <unordered_map>

namespace mbootcore {
namespace firmware {

class DirectoryFirmwareReader : public IFirmwareReader {
public:
    DirectoryFirmwareReader() = default;
    Result<std::unique_ptr<FirmwarePackage>> read(const std::string& path) override;
    bool canRead(const std::string& path) const override;
    std::string name() const override { return "DirectoryReader"; }
};

class RawFirmwareReader : public IFirmwareReader {
public:
    explicit RawFirmwareReader(const std::string& partitionName = "raw");
    Result<std::unique_ptr<FirmwarePackage>> read(const std::string& path) override;
    bool canRead(const std::string& path) const override;
    std::string name() const override { return "RawReader"; }
private:
    std::string m_partitionName;
};

class ZipFirmwareReader : public IFirmwareReader {
public:
    ZipFirmwareReader() = default;
    Result<std::unique_ptr<FirmwarePackage>> read(const std::string& path) override;
    bool canRead(const std::string& path) const override;
    std::string name() const override { return "ZipReader"; }
};

} // namespace firmware
} // namespace mbootcore
