#pragma once

#include <cstdint>

#ifdef _WIN32
#include <winsock2.h>

namespace mbootcore {
namespace platform {

class WinSockRuntime {
public:
    WinSockRuntime();
    ~WinSockRuntime();

    WinSockRuntime(const WinSockRuntime&) = delete;
    WinSockRuntime& operator=(const WinSockRuntime&) = delete;
    WinSockRuntime(WinSockRuntime&&) = delete;
    WinSockRuntime& operator=(WinSockRuntime&&) = delete;

    bool isInitialized() const noexcept { return m_ok; }

private:
    bool m_ok;
    static int refCount_;
    static bool winsockInitialized_;
};

} // namespace platform
} // namespace mbootcore

#endif // _WIN32
