#pragma once

#include <mbootcore/firmware/FirmwareTypes.hpp>
#include <string>
#include <memory>

namespace mbootcore {
namespace firmware {

class IFirmwareReader {
public:
    virtual ~IFirmwareReader() = default;
    virtual Result<std::unique_ptr<FirmwarePackage>> read(const std::string& path) = 0;
    virtual bool canRead(const std::string& path) const = 0;
    virtual std::string name() const = 0;
};

} // namespace firmware
} // namespace mbootcore
