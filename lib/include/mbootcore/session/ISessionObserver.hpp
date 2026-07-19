#pragma once

#include <mbootcore/session/SessionTypes.hpp>
#include <mbootcore/domain/Error.hpp>

namespace mbootcore {
namespace session {

class DeviceSession;

class ISessionObserver {
public:
    virtual ~ISessionObserver() = default;

    virtual void onStateChanged(DeviceSession& session, SessionState oldState, SessionState newState) = 0;

    virtual void onProgress(DeviceSession& session, uint64_t transferred, uint64_t total) = 0;

    virtual void onError(DeviceSession& session, ErrorCode error, const std::string& message) = 0;

    virtual void onCompleted(DeviceSession& session, const std::string& operation) = 0;

    virtual void onDisconnected(DeviceSession& session) = 0;

    virtual void onOperationStarted(DeviceSession& session, const std::string& operation) = 0;

    virtual void onOperationFinished(DeviceSession& session, const std::string& operation,
                                      bool success, std::chrono::milliseconds duration) = 0;
};

} // namespace session
} // namespace mbootcore
