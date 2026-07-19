#pragma once

#include "mbootcore/domain/Types.hpp"
#include "mbootcore/domain/Error.hpp"
#include "mbootcore/elf/ElfModels.hpp"

#include <memory>
#include <functional>

namespace mbootcore {
namespace elf {

enum class ProgrammerState : uint8_t {
    Idle,
    ImageLoaded,
    ImageVerified,
    TransferReady,
    Transferring,
    Executing,
    Completed,
    Failed,
    Cancelled
};

struct ProgrammerProgress {
    ProgrammerState state{ProgrammerState::Idle};
    size_t bytesTransferred{0};
    size_t totalBytes{0};
    std::string stage;
};

using ProgrammerProgressCallback = std::function<void(const ProgrammerProgress&)>;

class IProgrammerExecutor {
public:
    virtual ~IProgrammerExecutor() = default;

    // Stage 1: Load the parsed ELF image
    virtual Result<void> loadImage(const MemoryImage& image) = 0;

    // Stage 2: Verify image integrity (hash, signature, etc.)
    virtual Result<void> verifyImage() = 0;

    // Stage 3: Prepare for execution (configure device, etc.)
    virtual Result<void> prepareExecution() = 0;

    // Stage 4: Transfer image to device memory
    virtual Result<void> transferImage() = 0;

    // Stage 5: Start execution at entry point
    virtual Result<void> startExecution() = 0;

    // Full pipeline: load → verify → prepare → transfer → start
    virtual Result<void> execute(const MemoryImage& image) = 0;

    // Control
    virtual void cancel() noexcept = 0;
    virtual ProgrammerState state() const noexcept = 0;
    virtual ProgrammerProgress progress() const = 0;

    // Progress callback
    virtual void setProgressCallback(ProgrammerProgressCallback callback) = 0;
};

} // namespace elf
} // namespace mbootcore
