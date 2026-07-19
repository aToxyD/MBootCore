#pragma once

#include <mbootcore/session/SessionTypes.hpp>
#include <mbootcore/session/ISessionObserver.hpp>
#include <mbootcore/session/SessionLogger.hpp>
#include <mbootcore/domain/DeviceTypes.hpp>
#include <mbootcore/generic/IFlashDevice.hpp>
#include <mbootcore/pipeline/BootPipeline.hpp>
#include <mbootcore/domain/Error.hpp>

#include <memory>
#include <vector>
#include <string>
#include <atomic>
#include <mutex>
#include <shared_mutex>

namespace mbootcore {
namespace session {

class DeviceSession {
public:
    DeviceSession(discovery::DeviceDescriptor descriptor,
                  std::unique_ptr<IFlashDevice> flashDevice,
                  std::unique_ptr<pipeline::BootPipeline> pipeline = nullptr);

    ~DeviceSession();

    DeviceSession(const DeviceSession&) = delete;
    DeviceSession& operator=(const DeviceSession&) = delete;
    DeviceSession(DeviceSession&&) noexcept;
    DeviceSession& operator=(DeviceSession&&) noexcept;

    // Lifecycle
    Result<void> connect();
    void disconnect() noexcept;
    Result<void> reconnect();

    // State
    SessionState state() const noexcept { return m_state; }
    std::string stateString() const;
    bool isConnected() const noexcept;
    bool isBusy() const noexcept;
    bool isTerminal() const noexcept {
        auto s = m_state.load();
        return s == SessionState::Finished || s == SessionState::Error || s == SessionState::Cancelled;
    }

    // Device operations
    Result<ByteBuffer> readMemory(uint64_t address, size_t size);
    Result<void> writeMemory(uint64_t address, const ByteBuffer& data);
    Result<void> eraseMemory(uint64_t address, size_t size);
    Result<void> resetDevice();

    // Partition operations
    Result<ByteBuffer> readPartition(const std::string& name);
    Result<void> writePartition(const std::string& name, const ByteBuffer& data);
    Result<void> erasePartition(const std::string& name);

    // Observer management
    void addObserver(ISessionObserver* observer);
    void addObserver(std::shared_ptr<ISessionObserver> observer);
    void removeObserver(ISessionObserver* observer);
    bool hasObserver(ISessionObserver* observer) const;
    std::size_t observerCount() const;

    // Cancellation
    void cancel() noexcept;
    bool isCancelled() const noexcept { return m_cancelled; }

    // Recovery
    Result<void> retry();
    Result<void> reset();

    // Statistics
    SessionStatistics statistics() const noexcept;
    std::vector<SessionOperation> history() const noexcept;

    // Configuration
    void setConfig(const SessionConfig& config) { m_config = config; }
    const SessionConfig& config() const noexcept { return m_config; }
    SessionConfig& config() noexcept { return m_config; }

    // Accessors
    const discovery::DeviceDescriptor& descriptor() const noexcept { return m_descriptor; }
    IFlashDevice* flashDevice() const noexcept { return m_flashDevice.get(); }
    pipeline::BootPipeline* pipeline() const noexcept { return m_pipeline.get(); }
    SessionLogger& logger() noexcept { return m_logger; }
    const SessionLogger& logger() const noexcept { return m_logger; }

    // Session identity
    std::string sessionId() const { return m_sessionId; }
    void setSessionId(const std::string& id) { m_sessionId = id; }

private:
    void setState(SessionState newState);
    void notifyStateChanged(SessionState oldState, SessionState newState);
    void notifyError(ErrorCode error, const std::string& message);
    void notifyCompleted(const std::string& operation);
    void notifyDisconnected();
    void notifyOperationStarted(const std::string& operation);
    void notifyOperationFinished(const std::string& operation, bool success, std::chrono::milliseconds duration);

    Result<void> executeWithStateCheck(std::function<Result<void>()> operation,
                                        const std::string& operationName,
                                        SessionState busyState,
                                        uint64_t bytesTransferred = 0);
    Result<ByteBuffer> executeReadWithStateCheck(std::function<Result<ByteBuffer>()> operation,
                                                   const std::string& operationName);

    void updateStats(const std::string& operation, uint64_t bytes);
    void addHistoryEntry(const std::string& name, bool success, uint64_t bytes);
    bool checkCancelled();
    std::vector<std::shared_ptr<ISessionObserver>> copyAliveObservers() const;

    std::string m_sessionId;
    discovery::DeviceDescriptor m_descriptor;
    std::unique_ptr<IFlashDevice> m_flashDevice;
    std::unique_ptr<pipeline::BootPipeline> m_pipeline;

    std::atomic<SessionState> m_state{SessionState::Disconnected};
    SessionStatistics m_stats;
    SessionConfig m_config;
    std::vector<SessionOperation> m_history;

    mutable std::vector<std::shared_ptr<ISessionObserver>> m_observers;
    mutable std::mutex m_observersMutex;
    mutable std::shared_mutex m_statsMutex;
    std::atomic<bool> m_cancelled{false};

    SessionLogger m_logger;
    std::chrono::steady_clock::time_point m_lastOperationTime;
};

} // namespace session
} // namespace mbootcore
