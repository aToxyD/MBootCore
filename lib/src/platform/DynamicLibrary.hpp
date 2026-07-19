#pragma once

#include <mbootcore/domain/Error.hpp>
#include <memory>
#include <string>

namespace mbootcore {
namespace platform {

class DynamicLibrary {
public:
    ~DynamicLibrary();

    DynamicLibrary(const DynamicLibrary&) = delete;
    DynamicLibrary& operator=(const DynamicLibrary&) = delete;
    DynamicLibrary(DynamicLibrary&&) noexcept;
    DynamicLibrary& operator=(DynamicLibrary&&) noexcept;

    Result<void*> symbol(const std::string& name) noexcept;
    bool isLoaded() const noexcept { return m_handle != nullptr; }
    std::string path() const noexcept { return m_path; }
    std::string lastError() const noexcept { return m_error; }

    static Result<std::unique_ptr<DynamicLibrary>> load(const std::string& path);

private:
    DynamicLibrary(void* handle, std::string path);
    void* m_handle;
    std::string m_path;
    std::string m_error;
};

} // namespace platform
} // namespace mbootcore
