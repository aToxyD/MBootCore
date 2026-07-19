#include <mbootcore/vendor/VendorSession.hpp>
#include <mbootcore/vendor/VendorPipeline.hpp>
#include <mbootcore/vendor/IVendor.hpp>

namespace mbootcore {
namespace vendor {

VendorSession::VendorSession()
    : m_runtime(std::make_unique<VendorRuntime>()) {}

VendorSession::VendorSession(std::unique_ptr<VendorRuntime> runtime)
    : m_runtime(std::move(runtime)) {}

VendorSession::~VendorSession() { (void)close(); }

Result<void> VendorSession::open(const std::string& vendorId) {
    if (m_open) {
        ++m_stats.failedSessions;
        return ErrorCode::TransportAlreadyOpen;
    }
    VendorContext ctx;
    ctx.vendorId = vendorId;
    auto initResult = m_runtime->initialize(ctx);
    if (!initResult.isOk()) {
        ++m_stats.failedSessions;
        return initResult;
    }
    auto loadResult = m_runtime->loadVendor(vendorId);
    if (!loadResult.isOk()) {
        ++m_stats.failedSessions;
        return loadResult;
    }
    m_pipeline = std::make_unique<VendorPipeline>(m_runtime->mutableContext());
    m_vendorId = vendorId;
    m_open = true;
    ++m_stats.successfulSessions;
    return {};
}

Result<void> VendorSession::close() noexcept {
    if (!m_open) return {};
    m_pipeline.reset();
    (void)m_runtime->shutdown();
    m_open = false;
    m_vendorId.clear();
    return {};
}

Result<void> VendorSession::reconnect() {
    auto id = m_vendorId;
    (void)close();
    return open(id);
}

Result<void> VendorSession::reset() {
    if (!m_open) return ErrorCode::InvalidState;
    return m_runtime->createRuntimeContext(m_vendorId);
}

VendorStatistics VendorSession::statistics() const {
    return m_stats;
}

VendorCapability VendorSession::capabilities() const {
    auto* vendor = m_runtime->activeVendor();
    return vendor ? vendor->capabilities() : VendorCapability::None;
}

std::string VendorSession::activeVendorId() const noexcept {
    return m_vendorId;
}

bool VendorSession::isOpen() const noexcept { return m_open; }
VendorRuntime& VendorSession::runtime() noexcept { return *m_runtime; }
VendorPipeline* VendorSession::pipeline() noexcept { return m_pipeline.get(); }

} // namespace vendor
} // namespace mbootcore
