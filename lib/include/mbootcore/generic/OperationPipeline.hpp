#pragma once

#include "mbootcore/generic/IFlashOperation.hpp"
#include "mbootcore/generic/IFlashDevice.hpp"
#include "mbootcore/generic/FlashCapability.hpp"

#include <chrono>

namespace mbootcore {

class OperationPipeline {
public:
    virtual ~OperationPipeline() = default;
    explicit OperationPipeline(IFlashDevice& device);

    Result<void> execute(IFlashOperation& op,
                         ProgressCallback callback = nullptr);

    void cancel() noexcept { m_cancelled = true; }
    bool isCancelled() const noexcept { return m_cancelled; }
    const ProgressInfo& lastProgress() const noexcept { return m_lastProgress; }

protected:
    virtual Result<void> validate(IFlashOperation& op);
    virtual Result<void> checkCapabilities(IFlashOperation& op);
    virtual Result<void> executeImpl(IFlashOperation& op,
                                     ProgressCallback callback);

    IFlashDevice& m_device;
    ProgressInfo m_lastProgress;
    bool m_cancelled{false};
    bool m_running{false};
};

} // namespace mbootcore
