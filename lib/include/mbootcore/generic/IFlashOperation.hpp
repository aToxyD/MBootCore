#pragma once

#include <mbootcore/domain/Types.hpp>
#include <mbootcore/domain/Error.hpp>
#include <mbootcore/generic/ProgressInfo.hpp>
#include <mbootcore/generic/FlashCapability.hpp>

namespace mbootcore {

class IFlashOperation {
public:
    virtual ~IFlashOperation() = default;

    virtual std::string_view name() const noexcept = 0;
    virtual FlashCapability requiredCapabilities() const noexcept = 0;
    virtual Result<void> validate() = 0;
    virtual Result<void> execute(ProgressCallback callback) = 0;
    virtual void cancel() noexcept = 0;
    virtual bool isCancelled() const noexcept = 0;
    virtual const ProgressInfo& progress() const noexcept = 0;
};

} // namespace mbootcore
