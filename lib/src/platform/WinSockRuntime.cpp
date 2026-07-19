#ifdef _WIN32
#include "WinSockRuntime.hpp"

namespace mbootcore {
namespace platform {

int WinSockRuntime::refCount_ = 0;
bool WinSockRuntime::winsockInitialized_ = false;

WinSockRuntime::WinSockRuntime()
    : m_ok(false) {
    if (!winsockInitialized_) {
        WSADATA wsaData;
        int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
        if (result == 0) {
            winsockInitialized_ = true;
            m_ok = true;
        }
    } else {
        m_ok = true;
    }
    if (m_ok) {
        ++refCount_;
    }
}

WinSockRuntime::~WinSockRuntime() {
    if (m_ok) {
        --refCount_;
        if (refCount_ == 0 && winsockInitialized_) {
            WSACleanup();
            winsockInitialized_ = false;
        }
    }
}

} // namespace platform
} // namespace mbootcore

#endif // _WIN32
