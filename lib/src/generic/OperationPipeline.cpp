#include "mbootcore/generic/OperationPipeline.hpp"

namespace mbootcore {

OperationPipeline::OperationPipeline(IFlashDevice& device)
    : m_device(device) {}

Result<void> OperationPipeline::execute(IFlashOperation& op,
                                         ProgressCallback callback) {
    if (m_running) return ErrorCode::InvalidState;
    if (m_cancelled) return ErrorCode::Cancelled;

    m_running = true;

    auto valResult = validate(op);
    if (!valResult) {
        m_running = false;
        return valResult;
    }

    auto capResult = checkCapabilities(op);
    if (!capResult) {
        m_running = false;
        return capResult;
    }

    auto execResult = executeImpl(op, std::move(callback));
    m_running = false;
    return execResult;
}

Result<void> OperationPipeline::validate(IFlashOperation& op) {
    return op.validate();
}

Result<void> OperationPipeline::checkCapabilities(IFlashOperation& op) {
    auto deviceCaps = m_device.capabilities();
    auto requiredCaps = op.requiredCapabilities();

    if ((static_cast<uint64_t>(deviceCaps) & static_cast<uint64_t>(requiredCaps))
        != static_cast<uint64_t>(requiredCaps)) {
        return ErrorCode::NotSupported;
    }
    return {};
}

Result<void> OperationPipeline::executeImpl(IFlashOperation& op,
                                             ProgressCallback callback) {
    return op.execute(std::move(callback));
}

} // namespace mbootcore
