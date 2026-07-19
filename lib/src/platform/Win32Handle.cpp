#ifdef _WIN32
#include "Win32Handle.hpp"

namespace mbootcore {
namespace platform {

Win32Handle::Win32Handle(HANDLE handle)
    : m_handle(handle) {}

Win32Handle::~Win32Handle() {
    if (isValid()) {
        CloseHandle(m_handle);
    }
}

Win32Handle::Win32Handle(Win32Handle&& other) noexcept
    : m_handle(other.m_handle) {
    other.m_handle = INVALID_HANDLE_VALUE;
}

Win32Handle& Win32Handle::operator=(Win32Handle&& other) noexcept {
    if (this != &other) {
        if (isValid()) {
            CloseHandle(m_handle);
        }
        m_handle = other.m_handle;
        other.m_handle = INVALID_HANDLE_VALUE;
    }
    return *this;
}

HANDLE Win32Handle::release() noexcept {
    HANDLE h = m_handle;
    m_handle = INVALID_HANDLE_VALUE;
    return h;
}

void Win32Handle::reset(HANDLE handle) {
    if (isValid()) {
        CloseHandle(m_handle);
    }
    m_handle = handle;
}

} // namespace platform
} // namespace mbootcore

#endif // _WIN32
