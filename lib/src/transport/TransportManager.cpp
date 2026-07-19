#include <mbootcore/transport/TransportManager.hpp>
#include <mbootcore/transport/TransportFactory.hpp>
#include <mutex>

namespace mbootcore {
namespace transport {

TransportManager::~TransportManager() {
    closeAll();
}

bool TransportManager::add(std::string id, std::unique_ptr<ITransport> transport) {
    std::unique_lock lock(m_mutex);
    if (!transport || m_transports.find(id) != m_transports.end()) {
        return false;
    }
    Entry entry;
    entry.transport = std::move(transport);
    m_transports[std::move(id)] = std::move(entry);
    return true;
}

bool TransportManager::remove(const std::string& id) {
    std::unique_lock lock(m_mutex);
    auto it = m_transports.find(id);
    if (it == m_transports.end()) return false;
    if (it->second.transport) {
        it->second.transport->close();
    }
    m_transports.erase(it);
    return true;
}

ITransport* TransportManager::get(const std::string& id) const {
    std::shared_lock lock(m_mutex);
    auto it = m_transports.find(id);
    return it != m_transports.end() ? it->second.transport.get() : nullptr;
}

ITransport* TransportManager::getOrCreate(const std::string& id, TransportType type) {
    std::unique_lock lock(m_mutex);
    auto it = m_transports.find(id);
    if (it != m_transports.end()) {
        return it->second.transport.get();
    }
    std::unique_ptr<ITransport> trans;
    switch (type) {
        case TransportType::USB: trans = TransportFactory::createUSB(); break;
        case TransportType::Serial: trans = TransportFactory::createSerial({}); break;
        case TransportType::TCP: trans = TransportFactory::createTCP({}); break;
        case TransportType::UDP: trans = TransportFactory::createUDP(); break;
        default: trans = TransportFactory::createMock(); break;
    }
    if (!trans) return nullptr;
    auto* raw = trans.get();
    Entry entry;
    entry.transport = std::move(trans);
    m_transports[id] = std::move(entry);
    return raw;
}

bool TransportManager::open(const std::string& id) {
    std::unique_lock lock(m_mutex);
    auto it = m_transports.find(id);
    if (it == m_transports.end() || !it->second.transport) return false;
    auto result = it->second.transport->open();
    lock.unlock();
    if (result.isOk()) {
        notify(id, TransportState::Open);
    }
    return result.isOk();
}

bool TransportManager::close(const std::string& id) {
    std::unique_lock lock(m_mutex);
    auto it = m_transports.find(id);
    if (it == m_transports.end() || !it->second.transport) return false;
    it->second.transport->close();
    lock.unlock();
    notify(id, TransportState::Closed);
    return true;
}

bool TransportManager::closeAll() {
    std::vector<std::string> toNotify;
    {
        std::unique_lock lock(m_mutex);
        for (auto& [id, entry] : m_transports) {
            if (entry.transport && entry.transport->isOpen()) {
                entry.transport->close();
                toNotify.push_back(id);
            }
        }
    }
    for (const auto& id : toNotify) {
        notify(id, TransportState::Closed);
    }
    return true;
}

bool TransportManager::reconnect(const std::string& id) {
    std::unique_lock lock(m_mutex);
    auto it = m_transports.find(id);
    if (it == m_transports.end() || !it->second.transport) return false;
    auto result = it->second.transport->reconnect();
    lock.unlock();
    if (result.isOk()) {
        notify(id, TransportState::Open);
    }
    return result.isOk();
}

size_t TransportManager::count() const {
    std::shared_lock lock(m_mutex);
    return m_transports.size();
}

size_t TransportManager::openCount() const {
    std::shared_lock lock(m_mutex);
    size_t c = 0;
    for (const auto& [id, entry] : m_transports)
        if (entry.transport && entry.transport->isOpen()) ++c;
    return c;
}

std::vector<std::string> TransportManager::ids() const {
    std::shared_lock lock(m_mutex);
    std::vector<std::string> r;
    r.reserve(m_transports.size());
    for (const auto& [id, entry] : m_transports) r.push_back(id);
    return r;
}

std::vector<std::string> TransportManager::openIds() const {
    std::shared_lock lock(m_mutex);
    std::vector<std::string> r;
    for (const auto& [id, entry] : m_transports)
        if (entry.transport && entry.transport->isOpen()) r.push_back(id);
    return r;
}

TransportStatistics TransportManager::totalStatistics() const {
    std::shared_lock lock(m_mutex);
    TransportStatistics t;
    for (const auto& [id, entry] : m_transports) {
        if (entry.transport) {
            auto s = entry.transport->statistics();
            t.bytesRead += s.bytesRead;
            t.bytesWritten += s.bytesWritten;
            t.readOperations += s.readOperations;
            t.writeOperations += s.writeOperations;
            t.reconnectCount += s.reconnectCount;
            t.timeoutCount += s.timeoutCount;
            t.errorCount += s.errorCount;
        }
    }
    return t;
}

void TransportManager::resetStatistics(const std::string& id) {
    (void)id;
    // Transport-level statistics cannot be individually reset
    // through the current ITransport interface.
}

void TransportManager::resetAllStatistics() {
    // Transport-level statistics cannot be individually reset
    // through the current ITransport interface.
}

void TransportManager::setCallback(TransportCallback cb) {
    std::unique_lock lock(m_mutex);
    m_callback = std::move(cb);
}

void TransportManager::notify(const std::string& id, TransportState state) {
    TransportCallback cb;
    {
        std::unique_lock lock(m_mutex);
        cb = m_callback;
    }
    if (cb) cb(id, state);
}

} // namespace transport
} // namespace mbootcore
