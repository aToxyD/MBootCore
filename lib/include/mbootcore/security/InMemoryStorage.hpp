#pragma once

#include <memory>
#include <string>
#include <vector>

#include <mbootcore/security/SecurityTypes.hpp>
#include <mbootcore/domain/Error.hpp>

namespace mbootcore { namespace security { namespace testing {

class InMemoryStorage {
public:
    InMemoryStorage();
    ~InMemoryStorage();
    InMemoryStorage(const InMemoryStorage&) = delete;
    InMemoryStorage& operator=(const InMemoryStorage&) = delete;
    InMemoryStorage(InMemoryStorage&&) noexcept;
    InMemoryStorage& operator=(InMemoryStorage&&) noexcept;

    Result<void> store(const std::string& key, const std::string& value);
    Result<std::string> retrieve(const std::string& key);
    Result<void> remove(const std::string& key);
    Result<bool> exists(const std::string& key) const;
    Result<void> clear();

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

} } }
