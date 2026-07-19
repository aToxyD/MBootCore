#pragma once

#include <mbootcore/vendor/VendorTypes.hpp>
#include <mbootcore/vendor/VendorContext.hpp>
#include <mbootcore/domain/Error.hpp>
#include <memory>
#include <string>
#include <functional>

namespace mbootcore {
namespace vendor {

class IVendor;

class VendorPipeline {
public:
    explicit VendorPipeline(VendorContext& context);
    ~VendorPipeline() = default;

    VendorPipeline(const VendorPipeline&) = delete;
    VendorPipeline& operator=(const VendorPipeline&) = delete;
    VendorPipeline(VendorPipeline&&) = delete;
    VendorPipeline& operator=(VendorPipeline&&) = delete;

    Result<void> execute(const std::string& vendorId);
    Result<void> executeWithWorkflow(const std::string& vendorId);
    Result<void> cancel();
    bool isRunning() const noexcept;
    std::string currentVendorId() const noexcept;

    using ProgressCallback = std::function<void(const std::string& stage, int percent)>;
    void setProgressCallback(ProgressCallback cb);

private:
    VendorContext& m_context;
    bool m_running{false};
    std::string m_currentVendorId;
    ProgressCallback m_progressCb;
};

} // namespace vendor
} // namespace mbootcore
