#pragma once

#ifdef _WIN32
#include <windows.h>

namespace mbootcore {
namespace platform {

class Win32Handle {
public:
    explicit Win32Handle(HANDLE handle = INVALID_HANDLE_VALUE);
    ~Win32Handle();

    Win32Handle(Win32Handle&& other) noexcept;
    Win32Handle& operator=(Win32Handle&& other) noexcept;

    Win32Handle(const Win32Handle&) = delete;
    Win32Handle& operator=(const Win32Handle&) = delete;

    HANDLE get() const noexcept { return m_handle; }
    HANDLE release() noexcept;
    void reset(HANDLE handle = INVALID_HANDLE_VALUE);

    bool isValid() const noexcept {
        return m_handle != INVALID_HANDLE_VALUE && m_handle != nullptr;
    }
    explicit operator bool() const noexcept { return isValid(); }

private:
    HANDLE m_handle;
};

} // namespace platform
} // namespace mbootcore

#endif // _WIN32
