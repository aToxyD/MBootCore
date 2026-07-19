#pragma once

#include <mbootcore/transport/TransportTypes.hpp>
#include <mbootcore/domain/ITransport.hpp>
#include <memory>
#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <vector>
#include <functional>

namespace mbootcore {
namespace transport {

/**
 * @brief Manages a collection of named transport instances.
 *
 * Provides lifecycle management (add, remove, open, close, reconnect)
 * for multiple transports identified by string keys. Thread-safe for
 * concurrent read access via shared_mutex.
 *
 * Non-copyable, non-movable.
 */
class TransportManager {
public:
    TransportManager() = default;
    ~TransportManager();
    TransportManager(const TransportManager&) = delete;
    TransportManager& operator=(const TransportManager&) = delete;
    TransportManager(TransportManager&&) = delete;
    TransportManager& operator=(TransportManager&&) = delete;

    /// @brief Registers a transport with the given ID. Returns false if ID already exists.
    bool add(std::string id, std::unique_ptr<ITransport> transport);

    /// @brief Removes and destroys the transport with the given ID.
    bool remove(const std::string& id);

    /// @brief Returns a pointer to the transport with the given ID, or nullptr.
    ITransport* get(const std::string& id) const;

    /// @brief Returns an existing transport or creates one of the specified type.
    ITransport* getOrCreate(const std::string& id, TransportType type);

    /// @brief Opens the transport with the given ID.
    bool open(const std::string& id);

    /// @brief Closes the transport with the given ID.
    bool close(const std::string& id);

    /// @brief Closes all managed transports.
    bool closeAll();

    /// @brief Reconnects the transport with the given ID (close + open).
    bool reconnect(const std::string& id);

    /// @brief Returns the total number of managed transports.
    size_t count() const;

    /// @brief Returns the number of currently open transports.
    size_t openCount() const;

    /// @brief Returns the IDs of all managed transports.
    std::vector<std::string> ids() const;

    /// @brief Returns the IDs of all currently open transports.
    std::vector<std::string> openIds() const;

    /// @brief Callback type for state change notifications.
    using TransportCallback = std::function<void(const std::string& id, TransportState state)>;

    /// @brief Registers a callback invoked on transport state changes.
    void setCallback(TransportCallback cb);

    /// @brief Returns aggregated statistics across all managed transports.
    TransportStatistics totalStatistics() const;

    /// @brief Resets statistics for a specific transport.
    void resetStatistics(const std::string& id);

    /// @brief Resets statistics for all managed transports.
    void resetAllStatistics();

private:
    struct Entry {
        std::unique_ptr<ITransport> transport;
    };
    mutable std::shared_mutex m_mutex;
    std::unordered_map<std::string, Entry> m_transports;
    TransportCallback m_callback;
    void notify(const std::string& id, TransportState state);
};

} // namespace transport
} // namespace mbootcore
