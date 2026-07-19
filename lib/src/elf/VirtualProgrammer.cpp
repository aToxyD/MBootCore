#include "mbootcore/elf/VirtualProgrammer.hpp"

#include <algorithm>

#ifdef _WIN32
#include <windows.h>
#else
#include <thread>
#include <chrono>
#endif

namespace mbootcore {
namespace elf {

void VirtualProgrammer::updateProgress(ProgrammerState state, size_t transferred, size_t total) {
    m_state = state;
    if (m_callback) {
        ProgrammerProgress p;
        p.state = state;
        p.bytesTransferred = transferred;
        p.totalBytes = total;
        m_callback(p);
    }
}

void VirtualProgrammer::checkCancelled() {
    if (m_cancelled) {
        m_state = ProgrammerState::Cancelled;
        updateProgress(ProgrammerState::Cancelled, 0, 0);
    }
}

void VirtualProgrammer::setProgressCallback(ProgrammerProgressCallback callback) {
    m_callback = std::move(callback);
}

ProgrammerProgress VirtualProgrammer::progress() const {
    ProgrammerProgress p;
    p.state = m_state.load();
    p.bytesTransferred = m_loadedImage ? m_loadedImage->totalFileSize : 0;
    p.totalBytes = p.bytesTransferred;
    return p;
}

Result<void> VirtualProgrammer::loadImage(const MemoryImage& image) {
    if (m_failOnLoad) {
        m_state = ProgrammerState::Failed;
        return ErrorCode::InvalidElf;
    }

    m_loadedImage = std::make_unique<MemoryImage>(image);
    updateProgress(ProgrammerState::ImageLoaded, 0, image.totalFileSize);
    return {};
}

Result<void> VirtualProgrammer::verifyImage() {
    checkCancelled();
    if (m_state == ProgrammerState::Cancelled) {
        return ErrorCode::Cancelled;
    }

    if (m_failOnVerify || !m_loadedImage) {
        m_state = ProgrammerState::Failed;
        return ErrorCode::InvalidElf;
    }

    updateProgress(ProgrammerState::ImageVerified, 0, m_loadedImage->totalFileSize);
    return {};
}

Result<void> VirtualProgrammer::prepareExecution() {
    checkCancelled();
    if (m_state == ProgrammerState::Cancelled) {
        return ErrorCode::Cancelled;
    }

    if (m_failOnPrepare || !m_loadedImage) {
        m_state = ProgrammerState::Failed;
        return ErrorCode::InvalidElf;
    }

    updateProgress(ProgrammerState::TransferReady, 0, m_loadedImage->totalFileSize);
    return {};
}

Result<void> VirtualProgrammer::transferImage() {
    if (m_failOnTransfer || !m_loadedImage) {
        m_state = ProgrammerState::Failed;
        return ErrorCode::InvalidElf;
    }

    if (m_simulateTimeout) {
        m_state = ProgrammerState::Failed;
        return ErrorCode::TransportTimeout;
    }

    updateProgress(ProgrammerState::Transferring, 0, m_loadedImage->totalFileSize);

    size_t total = static_cast<size_t>(m_loadedImage->totalFileSize);
    size_t transferred = 0;

    while (transferred < total) {
        checkCancelled();
        if (m_state == ProgrammerState::Cancelled) {
            return ErrorCode::Cancelled;
        }

        size_t chunk = (std::min)(total - transferred, m_transferChunkSize);
        transferred += chunk;

        updateProgress(ProgrammerState::Transferring, transferred, total);

        if (m_transferDelayMs > 0) {
#ifdef _WIN32
            Sleep(static_cast<DWORD>(m_transferDelayMs));
#else
            std::this_thread::sleep_for(std::chrono::milliseconds(m_transferDelayMs));
#endif
        }
    }

    updateProgress(ProgrammerState::Executing, transferred, total);
    return {};
}

Result<void> VirtualProgrammer::startExecution() {
    checkCancelled();
    if (m_state == ProgrammerState::Cancelled) {
        return ErrorCode::Cancelled;
    }

    if (m_failOnStart || !m_loadedImage) {
        m_state = ProgrammerState::Failed;
        return ErrorCode::InvalidElf;
    }

    m_executed = true;
    updateProgress(ProgrammerState::Completed, m_loadedImage->totalFileSize,
                   m_loadedImage->totalFileSize);
    return {};
}

Result<void> VirtualProgrammer::execute(const MemoryImage& image) {
    MBOOT_TRY(loadImage(image));
    MBOOT_TRY(verifyImage());
    MBOOT_TRY(prepareExecution());
    MBOOT_TRY(transferImage());
    return startExecution();
}

void VirtualProgrammer::cancel() noexcept {
    m_cancelled = true;
    m_state = ProgrammerState::Cancelled;
}

} // namespace elf
} // namespace mbootcore
