#pragma once

#include "mbootcore/elf/IProgrammerExecutor.hpp"

#include <atomic>
#include <memory>

namespace mbootcore {
namespace elf {

class VirtualProgrammer : public IProgrammerExecutor {
public:
    VirtualProgrammer() = default;

    Result<void> loadImage(const MemoryImage& image) override;
    Result<void> verifyImage() override;
    Result<void> prepareExecution() override;
    Result<void> transferImage() override;
    Result<void> startExecution() override;
    Result<void> execute(const MemoryImage& image) override;

    void cancel() noexcept override;
    ProgrammerState state() const noexcept override { return m_state; }
    ProgrammerProgress progress() const override;

    void setProgressCallback(ProgrammerProgressCallback callback) override;

    // Configuration for test scenarios
    void setFailOnLoad(bool fail) noexcept { m_failOnLoad = fail; }
    void setFailOnVerify(bool fail) noexcept { m_failOnVerify = fail; }
    void setFailOnPrepare(bool fail) noexcept { m_failOnPrepare = fail; }
    void setFailOnTransfer(bool fail) noexcept { m_failOnTransfer = fail; }
    void setFailOnStart(bool fail) noexcept { m_failOnStart = fail; }
    void setTransferDelayMs(int ms) noexcept { m_transferDelayMs = ms; }
    void setTransferChunkSize(size_t bytes) noexcept { m_transferChunkSize = bytes; }

    // Simulate timeout during transfer
    void simulateTimeout(bool enable) noexcept { m_simulateTimeout = enable; }

    // Query
    const MemoryImage* loadedImage() const noexcept { return m_loadedImage.get(); }
    bool wasExecuted() const noexcept { return m_executed; }

private:
    std::atomic<ProgrammerState> m_state{ProgrammerState::Idle};
    std::unique_ptr<MemoryImage> m_loadedImage;
    ProgrammerProgressCallback m_callback;
    std::atomic<bool> m_cancelled{false};
    std::atomic<bool> m_executed{false};

    bool m_failOnLoad{false};
    bool m_failOnVerify{false};
    bool m_failOnPrepare{false};
    bool m_failOnTransfer{false};
    bool m_failOnStart{false};
    int m_transferDelayMs{0};
    size_t m_transferChunkSize{4096};
    bool m_simulateTimeout{false};

    void updateProgress(ProgrammerState state, size_t transferred, size_t total);
    void checkCancelled();
};

} // namespace elf
} // namespace mbootcore
