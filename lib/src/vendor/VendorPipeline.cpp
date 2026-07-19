#include <mbootcore/vendor/VendorPipeline.hpp>
#include <mbootcore/vendor/IVendor.hpp>
#include <mbootcore/vendor/VendorRegistry.hpp>
#include <mbootcore/pipeline/BootPipeline.hpp>
#include <mbootcore/session/DeviceSession.hpp>
#include <mbootcore/workflow/WorkflowEngine.hpp>

namespace mbootcore {
namespace vendor {

VendorPipeline::VendorPipeline(VendorContext& context)
    : m_context(context) {}

Result<void> VendorPipeline::execute(const std::string& vendorId) {
    if (m_running) return ErrorCode::TransportBusy;
    m_running = true;
    m_currentVendorId = vendorId;
    if (m_progressCb) m_progressCb("connect", 0);
    if (m_context.deviceSession) {
        auto result = m_context.deviceSession->connect();
        if (!result.isOk()) { m_running = false; return result; }
    }
    if (m_progressCb) m_progressCb("boot", 50);
    if (m_context.bootPipeline) {
        auto result = m_context.bootPipeline->run();
        if (!result.isOk()) { m_running = false; return result; }
    }
    if (m_progressCb) m_progressCb("complete", 100);
    m_running = false;
    return {};
}

Result<void> VendorPipeline::executeWithWorkflow(const std::string& vendorId) {
    if (m_running) return ErrorCode::TransportBusy;
    m_running = true;
    m_currentVendorId = vendorId;
    if (m_progressCb) m_progressCb("workflow", 0);
    if (m_context.workflowEngine) {
        auto result = m_context.workflowEngine->run();
        if (!result.isOk()) { m_running = false; return result; }
    }
    if (m_progressCb) m_progressCb("workflow_complete", 100);
    m_running = false;
    return {};
}

Result<void> VendorPipeline::cancel() {
    if (!m_running) return ErrorCode::InvalidState;
    if (m_context.deviceSession) m_context.deviceSession->disconnect();
    if (m_context.bootPipeline) m_context.bootPipeline->cancel();
    if (m_context.workflowEngine) m_context.workflowEngine->cancel();
    m_running = false;
    return {};
}

bool VendorPipeline::isRunning() const noexcept { return m_running; }
std::string VendorPipeline::currentVendorId() const noexcept { return m_currentVendorId; }

void VendorPipeline::setProgressCallback(ProgressCallback cb) {
    m_progressCb = std::move(cb);
}

} // namespace vendor
} // namespace mbootcore
