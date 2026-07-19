#include "platform/DynamicLibrary.hpp"

#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif

namespace mbootcore {
namespace platform {

DynamicLibrary::DynamicLibrary(void* handle, std::string path)
    : m_handle(handle), m_path(std::move(path)) {}

DynamicLibrary::~DynamicLibrary() {
    if (m_handle) {
#ifdef _WIN32
        FreeLibrary(static_cast<HMODULE>(m_handle));
#else
        dlclose(m_handle);
#endif
    }
}

DynamicLibrary::DynamicLibrary(DynamicLibrary&& other) noexcept
    : m_handle(other.m_handle)
    , m_path(std::move(other.m_path))
    , m_error(std::move(other.m_error)) {
    other.m_handle = nullptr;
}

DynamicLibrary& DynamicLibrary::operator=(DynamicLibrary&& other) noexcept {
    if (this != &other) {
        if (m_handle) {
#ifdef _WIN32
            FreeLibrary(static_cast<HMODULE>(m_handle));
#else
            dlclose(m_handle);
#endif
        }
        m_handle = other.m_handle;
        m_path = std::move(other.m_path);
        m_error = std::move(other.m_error);
        other.m_handle = nullptr;
    }
    return *this;
}

Result<std::unique_ptr<DynamicLibrary>> DynamicLibrary::load(const std::string& path) {
#ifdef _WIN32
    int wlen = MultiByteToWideChar(CP_UTF8, 0, path.c_str(), -1, nullptr, 0);
    std::wstring wpath(static_cast<size_t>(wlen), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, path.c_str(), -1, &wpath[0], wlen);
    HMODULE mod = LoadLibraryW(wpath.c_str());
    if (!mod) {
        return ErrorCode::PluginLoadFailed;
    }
    auto lib = std::unique_ptr<DynamicLibrary>(new DynamicLibrary(mod, path));
    return lib;
#else
    void* handle = dlopen(path.c_str(), RTLD_NOW | RTLD_LOCAL);
    if (!handle) {
        auto lib = std::unique_ptr<DynamicLibrary>(new DynamicLibrary(nullptr, path));
        lib->m_error = dlerror();
        return ErrorCode::PluginLoadFailed;
    }
    auto lib = std::unique_ptr<DynamicLibrary>(new DynamicLibrary(handle, path));
    return lib;
#endif
}

Result<void*> DynamicLibrary::symbol(const std::string& name) noexcept {
    if (!m_handle) {
        return ErrorCode::PluginNotLoaded;
    }
#ifdef _WIN32
    void* sym = reinterpret_cast<void*>(
        GetProcAddress(static_cast<HMODULE>(m_handle), name.c_str()));
#else
    void* sym = dlsym(m_handle, name.c_str());
#endif
    if (!sym) {
        m_error = "Symbol not found: " + name;
        return ErrorCode::PluginLoadFailed;
    }
    return sym;
}

} // namespace platform
} // namespace mbootcore
