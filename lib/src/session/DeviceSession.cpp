#include <mbootcore/session/DeviceSession.hpp>

#include <sstream>

namespace mbootcore {
namespace session {

DeviceSession::DeviceSession(discovery::DeviceDescriptor descriptor,
                              std::unique_ptr<IFlashDevice> flashDevice,
                              std::unique_ptr<pipeline::BootPipeline> pipeline)
    : m_descriptor(std::move(descriptor))
    , m_flashDevice(std::move(flashDevice))
    , m_pipeline(std::move(pipeline))
    , m_logger(m_descriptor.friendlyName.empty() ? "Session" : m_descriptor.friendlyName)
{
    {
        std::unique_lock lock(m_statsMutex);
        m_stats.startTime = std::chrono::steady_clock::now();
        m_lastOperationTime = m_stats.startTime;
    }

    std::ostringstream sid;
    if (m_descriptor.usbVid) {
        sid << "usb:" << std::hex << m_descriptor.usbVid << ":" << m_descriptor.usbPid;
    } else if (!m_descriptor.serialPort.empty()) {
        sid << "serial:" << m_descriptor.serialPort;
    } else if (!m_descriptor.tcpHost.empty()) {
        sid << "tcp:" << m_descriptor.tcpHost << ":" << m_descriptor.tcpPort;
    } else {
        sid << "session:" << reinterpret_cast<uintptr_t>(this);
    }
    m_sessionId = sid.str();

    m_logger.info("Session", "Created session: " + m_sessionId);
}

DeviceSession::~DeviceSession() {
    disconnect();
    m_logger.info("Session", "Destroyed session: " + m_sessionId);
}

DeviceSession::DeviceSession(DeviceSession&& other) noexcept
    : m_sessionId(std::move(other.m_sessionId))
    , m_descriptor(std::move(other.m_descriptor))
    , m_flashDevice(std::move(other.m_flashDevice))
    , m_pipeline(std::move(other.m_pipeline))
    , m_stats(std::move(other.m_stats))
    , m_config(std::move(other.m_config))
    , m_history(std::move(other.m_history))
    , m_observers(std::move(other.m_observers))
    , m_logger(std::move(other.m_logger))
    , m_lastOperationTime(std::move(other.m_lastOperationTime))
{
    m_state.store(other.m_state.load());
    m_cancelled.store(other.m_cancelled.load());
}

DeviceSession& DeviceSession::operator=(DeviceSession&& other) noexcept {
    if (this != &other) {
        disconnect();
        m_sessionId = std::move(other.m_sessionId);
        m_descriptor = std::move(other.m_descriptor);
        m_flashDevice = std::move(other.m_flashDevice);
        m_pipeline = std::move(other.m_pipeline);
        m_stats = std::move(other.m_stats);
        m_config = std::move(other.m_config);
        m_history = std::move(other.m_history);
        m_observers = std::move(other.m_observers);
        m_logger = std::move(other.m_logger);
        m_lastOperationTime = std::move(other.m_lastOperationTime);
        m_state.store(other.m_state.load());
        m_cancelled.store(other.m_cancelled.load());
    }
    return *this;
}

Result<void> DeviceSession::connect() {
    if (m_state.load() == SessionState::Ready || m_state.load() == SessionState::Connected) {
        return {};
    }
    if (isTerminal()) {
        return ErrorCode::InvalidState;
    }

    m_cancelled = false;
    setState(SessionState::Connecting);
    notifyOperationStarted("connect");

    if (m_flashDevice) {
        auto result = m_flashDevice->open();
        if (result.isError()) {
            setState(SessionState::Error);
            notifyError(result.error(), "Failed to open flash device");
            notifyOperationFinished("connect", false, std::chrono::milliseconds(0));
            return result;
        }
    }

    setState(SessionState::Ready);
    {
        std::unique_lock lock(m_statsMutex);
        m_stats.connectionTime = std::chrono::steady_clock::now();
    }
    m_logger.info("Session", "Device connected and ready");
    notifyCompleted("connect");
    notifyOperationFinished("connect", true, std::chrono::milliseconds(0));
    return {};
}

void DeviceSession::disconnect() noexcept {
    auto oldState = m_state.load();
    if (oldState == SessionState::Disconnected || oldState == SessionState::Finished) return;

    m_cancelled = true;

    if (m_flashDevice) {
        m_flashDevice->close();
    }

    setState(SessionState::Disconnected);
    {
        std::unique_lock lock(m_statsMutex);
        m_stats.elapsedTime = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - m_stats.startTime);
    }
    m_logger.info("Session", "Device disconnected");
    notifyDisconnected();
}

Result<void> DeviceSession::reconnect() {
    disconnect();
    notifyOperationStarted("reconnect");
    auto result = connect();
    notifyOperationFinished("reconnect", result.isOk(), std::chrono::milliseconds(0));
    if (result.isOk()) {
        m_logger.info("Session", "Device reconnected");
    }
    return result;
}

std::string DeviceSession::stateString() const {
    switch (m_state.load()) {
        case SessionState::Disconnected: return "Disconnected";
        case SessionState::Discovered: return "Discovered";
        case SessionState::Negotiated: return "Negotiated";
        case SessionState::Connected: return "Connected";
        case SessionState::Connecting: return "Connecting";
        case SessionState::Initializing: return "Initializing";
        case SessionState::Ready: return "Ready";
        case SessionState::Busy: return "Busy";
        case SessionState::Reading: return "Reading";
        case SessionState::Writing: return "Writing";
        case SessionState::Erasing: return "Erasing";
        case SessionState::Resetting: return "Resetting";
        case SessionState::Finished: return "Finished";
        case SessionState::Error: return "Error";
        case SessionState::Cancelled: return "Cancelled";
    }
    return "Unknown";
}

bool DeviceSession::isConnected() const noexcept {
    auto s = m_state.load();
    return s == SessionState::Connected || s == SessionState::Ready ||
           s == SessionState::Busy || s == SessionState::Reading ||
           s == SessionState::Writing || s == SessionState::Erasing;
}

bool DeviceSession::isBusy() const noexcept {
    auto s = m_state.load();
    return s == SessionState::Busy || s == SessionState::Reading ||
           s == SessionState::Writing || s == SessionState::Erasing ||
           s == SessionState::Resetting || s == SessionState::Initializing;
}

Result<ByteBuffer> DeviceSession::readMemory(uint64_t address, size_t size) {
    if (!m_flashDevice) return ErrorCode::NotSupported;

    return executeReadWithStateCheck(
        [this, address, size]() { return m_flashDevice->readMemory(address, size); },
        "readMemory");
}

Result<void> DeviceSession::writeMemory(uint64_t address, const ByteBuffer& data) {
    if (!m_flashDevice) return ErrorCode::NotSupported;

    return executeWithStateCheck(
        [this, address, &data]() { return m_flashDevice->writeMemory(address, data); },
        "writeMemory", SessionState::Writing, data.size());
}

Result<void> DeviceSession::eraseMemory(uint64_t address, size_t size) {
    if (!m_flashDevice) return ErrorCode::NotSupported;

    return executeWithStateCheck(
        [this, address, size]() { return m_flashDevice->eraseMemory(address, size); },
        "eraseMemory", SessionState::Erasing, size);
}

Result<void> DeviceSession::resetDevice() {
    if (!m_flashDevice) return ErrorCode::NotSupported;

    return executeWithStateCheck(
        [this]() { return m_flashDevice->reset(); },
        "resetDevice", SessionState::Resetting);
}

Result<ByteBuffer> DeviceSession::readPartition(const std::string& name) {
    if (!m_flashDevice) return ErrorCode::NotSupported;

    return executeReadWithStateCheck(
        [this, &name]() { return m_flashDevice->readPartition(name); },
        "readPartition:" + name);
}

Result<void> DeviceSession::writePartition(const std::string& name, const ByteBuffer& data) {
    if (!m_flashDevice) return ErrorCode::NotSupported;

    return executeWithStateCheck(
        [this, &name, &data]() { return m_flashDevice->writePartition(name, data); },
        "writePartition:" + name, SessionState::Writing);
}

Result<void> DeviceSession::erasePartition(const std::string& name) {
    if (!m_flashDevice) return ErrorCode::NotSupported;

    return executeWithStateCheck(
        [this, &name]() { return m_flashDevice->erasePartition(name); },
        "erasePartition:" + name, SessionState::Erasing);
}

void DeviceSession::addObserver(ISessionObserver* observer) {
    if (!observer) return;
    std::lock_guard<std::mutex> lock(m_observersMutex);
    for (auto& sp : m_observers) {
        if (sp.get() == observer) return;
    }
    m_observers.push_back(std::shared_ptr<ISessionObserver>(observer, [](ISessionObserver*) {}));
}

void DeviceSession::addObserver(std::shared_ptr<ISessionObserver> observer) {
    if (!observer) return;
    std::lock_guard<std::mutex> lock(m_observersMutex);
    m_observers.push_back(std::move(observer));
}

void DeviceSession::removeObserver(ISessionObserver* observer) {
    std::lock_guard<std::mutex> lock(m_observersMutex);
    for (auto it = m_observers.begin(); it != m_observers.end(); ) {
        if (it->get() == observer) {
            it = m_observers.erase(it);
        } else {
            ++it;
        }
    }
}

bool DeviceSession::hasObserver(ISessionObserver* observer) const {
    std::lock_guard<std::mutex> lock(m_observersMutex);
    for (auto& sp : m_observers) {
        if (sp.get() == observer) return true;
    }
    return false;
}

std::size_t DeviceSession::observerCount() const {
    std::lock_guard<std::mutex> lock(m_observersMutex);
    return m_observers.size();
}

void DeviceSession::cancel() noexcept {
    m_cancelled = true;
    setState(SessionState::Cancelled);
    if (m_flashDevice) {
        m_flashDevice->cancel();
    }
}

Result<void> DeviceSession::retry() {
    if (m_state.load() != SessionState::Error) {
        return ErrorCode::InvalidState;
    }

    {
        std::unique_lock lock(m_statsMutex);
        ++m_stats.retries;
    }
    m_logger.info("Session", "Retrying session");
    return reconnect();
}

Result<void> DeviceSession::reset() {
    disconnect();
    m_logger.info("Session", "Session reset");
    return {};
}

void DeviceSession::setState(SessionState newState) {
    auto oldState = m_state.exchange(newState);
    if (oldState != newState) {
        notifyStateChanged(oldState, newState);
    }
}

void DeviceSession::notifyStateChanged(SessionState oldState, SessionState newState) {
    auto observers = copyAliveObservers();
    for (const auto& observer : observers) {
        observer->onStateChanged(*this, oldState, newState);
    }
}

void DeviceSession::notifyError(ErrorCode error, const std::string& message) {
    m_logger.error("Session", message);
    auto observers = copyAliveObservers();
    for (const auto& observer : observers) {
        observer->onError(*this, error, message);
    }
}

void DeviceSession::notifyCompleted(const std::string& operation) {
    auto observers = copyAliveObservers();
    for (const auto& observer : observers) {
        observer->onCompleted(*this, operation);
    }
}

void DeviceSession::notifyDisconnected() {
    auto observers = copyAliveObservers();
    for (const auto& observer : observers) {
        observer->onDisconnected(*this);
    }
}

void DeviceSession::notifyOperationStarted(const std::string& operation) {
    m_logger.info("Session", "Operation started: " + operation);
    auto observers = copyAliveObservers();
    for (const auto& observer : observers) {
        observer->onOperationStarted(*this, operation);
    }
}

void DeviceSession::notifyOperationFinished(const std::string& operation, bool success,
                                              std::chrono::milliseconds duration) {
    m_logger.info("Session", "Operation finished: " + operation + " success=" + (success ? "true" : "false"));
    auto observers = copyAliveObservers();
    for (const auto& observer : observers) {
        observer->onOperationFinished(*this, operation, success, duration);
    }
}

bool DeviceSession::checkCancelled() {
    if (m_cancelled) {
        setState(SessionState::Cancelled);
        return true;
    }
    return false;
}

void DeviceSession::updateStats(const std::string& operation, uint64_t bytes) {
    auto now = std::chrono::steady_clock::now();
    std::unique_lock lock(m_statsMutex);
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_lastOperationTime);

    if (operation.find("read") != std::string::npos) {
        m_stats.bytesRead += bytes;
        ++m_stats.readOps;
        if (elapsed.count() > 0) {
            double bps = static_cast<double>(bytes) * 1000.0 / static_cast<double>(elapsed.count());
            m_stats.averageReadBps = m_stats.averageReadBps * 0.7 + bps * 0.3;
            if (bps > m_stats.maxReadBps) m_stats.maxReadBps = bps;
        }
    } else if (operation.find("write") != std::string::npos) {
        m_stats.bytesWritten += bytes;
        ++m_stats.writeOps;
        if (elapsed.count() > 0) {
            double bps = static_cast<double>(bytes) * 1000.0 / static_cast<double>(elapsed.count());
            m_stats.averageWriteBps = m_stats.averageWriteBps * 0.7 + bps * 0.3;
            if (bps > m_stats.maxWriteBps) m_stats.maxWriteBps = bps;
        }
    } else if (operation.find("erase") != std::string::npos) {
        m_stats.bytesErased += bytes;
        ++m_stats.eraseOps;
    }

    m_stats.elapsedTime = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_stats.startTime);
    m_lastOperationTime = now;
}

void DeviceSession::addHistoryEntry(const std::string& name, bool success, uint64_t bytes) {
    if (!m_config.enableHistory) return;

    SessionOperation op;
    op.name = name;
    std::unique_lock lock(m_statsMutex);
    op.startTime = m_lastOperationTime;
    op.duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - op.startTime);
    op.bytesTransferred = bytes;
    op.success = success;
    m_history.push_back(std::move(op));
}

Result<void> DeviceSession::executeWithStateCheck(
    std::function<Result<void>()> operation,
    const std::string& operationName,
    SessionState busyState,
    uint64_t bytesTransferred)
{
    if (checkCancelled()) return ErrorCode::Cancelled;
    if (!isConnected()) return ErrorCode::DeviceDisconnected;
    if (isBusy()) return ErrorCode::InvalidState;

    setState(busyState);
    notifyOperationStarted(operationName);

    auto opStart = std::chrono::steady_clock::now();
    auto result = operation();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - opStart);

    if (result.isOk()) {
        setState(SessionState::Ready);
        updateStats(operationName, bytesTransferred);
        addHistoryEntry(operationName, true, bytesTransferred);
        notifyCompleted(operationName);
        notifyOperationFinished(operationName, true, duration);
    } else {
        {
            std::unique_lock lock(m_statsMutex);
            ++m_stats.failures;
        }
        setState(SessionState::Error);
        notifyError(result.error(), "Operation failed: " + operationName);
        addHistoryEntry(operationName, false, bytesTransferred);
        notifyOperationFinished(operationName, false, duration);
    }

    return result;
}

Result<ByteBuffer> DeviceSession::executeReadWithStateCheck(
    std::function<Result<ByteBuffer>()> operation,
    const std::string& operationName)
{
    if (checkCancelled()) return ErrorCode::Cancelled;
    if (!isConnected()) return ErrorCode::DeviceDisconnected;
    if (isBusy()) return ErrorCode::InvalidState;

    setState(SessionState::Reading);
    notifyOperationStarted(operationName);

    auto opStart = std::chrono::steady_clock::now();
    auto result = operation();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - opStart);

    if (result.isOk()) {
        setState(SessionState::Ready);
        auto& data = result.value();
        updateStats(operationName, data.size());
        addHistoryEntry(operationName, true, data.size());
        notifyCompleted(operationName);
        notifyOperationFinished(operationName, true, duration);
    } else {
        {
            std::unique_lock lock(m_statsMutex);
            ++m_stats.failures;
        }
        setState(SessionState::Error);
        notifyError(result.error(), "Read failed: " + operationName);
        addHistoryEntry(operationName, false, 0);
        notifyOperationFinished(operationName, false, duration);
    }

    return result;
}

SessionStatistics DeviceSession::statistics() const noexcept {
    std::shared_lock lock(m_statsMutex);
    return m_stats;
}

std::vector<SessionOperation> DeviceSession::history() const noexcept {
    std::shared_lock lock(m_statsMutex);
    return m_history;
}

std::vector<std::shared_ptr<ISessionObserver>> DeviceSession::copyAliveObservers() const {
    std::lock_guard<std::mutex> lock(m_observersMutex);
    return m_observers;
}

} // namespace session
} // namespace mbootcore
