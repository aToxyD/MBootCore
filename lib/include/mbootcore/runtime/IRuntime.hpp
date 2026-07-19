#pragma once

#include <mbootcore/domain/Error.hpp>
#include <mbootcore/runtime/Services.hpp>
#include <mbootcore/runtime/RuntimeObserver.hpp>
#include <mbootcore/runtime/RuntimeCallbacks.hpp>

#include <memory>
#include <string>
#include <vector>

namespace mbootcore {
namespace runtime {

class IRuntime {
public:
    virtual ~IRuntime() = default;

    virtual Result<void> initialize() = 0;

    virtual void shutdown() noexcept = 0;

    virtual void pause() = 0;

    virtual void resume() = 0;

    virtual Result<void> reset() = 0;

    virtual Services& services() = 0;

    virtual void addObserver(RuntimeObserver* observer) = 0;

    virtual void removeObserver(RuntimeObserver* observer) = 0;

    virtual void setCallbacks(const RuntimeCallbacks& callbacks) = 0;

    virtual RuntimeCallbacks& callbacks() noexcept = 0;

    virtual std::string version() const = 0;

    virtual std::vector<std::string> capabilities() const = 0;

    virtual RuntimeStatistics statistics() const = 0;

    virtual RuntimeHealth health() const = 0;
};

} // namespace runtime
} // namespace mbootcore
