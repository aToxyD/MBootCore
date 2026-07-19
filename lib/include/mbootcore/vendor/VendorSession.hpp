#pragma once

#include <mbootcore/vendor/VendorTypes.hpp>
#include <mbootcore/vendor/VendorRuntime.hpp>
#include <mbootcore/vendor/VendorContext.hpp>
#include <mbootcore/session/SessionTypes.hpp>
#include <mbootcore/domain/Error.hpp>
#include <memory>
#include <string>

namespace mbootcore {
namespace vendor {

class VendorPipeline;

class VendorSession {
public:
    VendorSession();
    explicit VendorSession(std::unique_ptr<VendorRuntime> runtime);
    ~VendorSession();

    VendorSession(const VendorSession&) = delete;
    VendorSession& operator=(const VendorSession&) = delete;
    VendorSession(VendorSession&&) = delete;
    VendorSession& operator=(VendorSession&&) = delete;

    Result<void> open(const std::string& vendorId);
    Result<void> close() noexcept;
    Result<void> reconnect();
    Result<void> reset();

    VendorStatistics statistics() const;
    VendorCapability capabilities() const;
    std::string activeVendorId() const noexcept;
    bool isOpen() const noexcept;

    VendorRuntime& runtime() noexcept;
    VendorPipeline* pipeline() noexcept;

private:
    mutable VendorStatistics m_stats;
    std::unique_ptr<VendorRuntime> m_runtime;
    std::unique_ptr<VendorPipeline> m_pipeline;
    std::string m_vendorId;
    bool m_open{false};
};

} // namespace vendor
} // namespace mbootcore
