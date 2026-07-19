#pragma once

#include <mbootcore/vendor/VendorTypes.hpp>
#include <mbootcore/vendor/VendorContext.hpp>
#include <mbootcore/firmware/FirmwareTypes.hpp>
#include <mbootcore/domain/Error.hpp>
#include <string>
#include <vector>
#include <memory>

namespace mbootcore {
namespace firmware { class FirmwarePackage; }
namespace loader { class ILoader; }
namespace gpt { class GPTTable; }

namespace vendor {

struct ResolvedPackage {
    std::unique_ptr<firmware::FirmwarePackage> package;
    std::string loaderPath;
    bool hasGpt{false};
    std::vector<std::string> partitionOrder;
};

class VendorPackageResolver {
public:
    explicit VendorPackageResolver(const VendorContext& context);
    ~VendorPackageResolver() = default;

    Result<ResolvedPackage> resolvePackage(const std::string& path);
    Result<std::string> resolveLoader(const std::string& vendorId);
    Result<void> resolveGptLayout(const std::string& vendorId);
    Result<std::vector<std::string>> resolvePartitionLayout(const std::string& vendorId);
    Result<std::vector<std::string>> resolveFlashingSequence(const std::string& vendorId);

private:
    [[maybe_unused]] const VendorContext& m_context;
};

} // namespace vendor
} // namespace mbootcore
