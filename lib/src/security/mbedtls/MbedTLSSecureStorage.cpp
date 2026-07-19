#ifdef MBOOTCORE_HAVE_MBEDTLS

#include "mbootcore/security/MbedTLSSecureStorage.hpp"

#include <map>
#include <mutex>
#include <algorithm>

namespace mbootcore { namespace security {

class MbedTLSSecureStorage final : public ISecureStorage {
public:
    Result<void> store(const std::string& key, const std::string& value) override {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_storage[key] = value;
        return Result<void>::Ok();
    }

    Result<std::string> retrieve(const std::string& key) override {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_storage.find(key);
        if (it == m_storage.end()) {
            return Result<std::string>::Error(ErrorCode::InvalidArgument);
        }
        return Result<std::string>::Ok(it->second);
    }

    Result<void> remove(const std::string& key) override {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_storage.erase(key);
        return Result<void>::Ok();
    }

    Result<bool> exists(const std::string& key) const override {
        std::lock_guard<std::mutex> lock(m_mutex);
        return Result<bool>::Ok(m_storage.find(key) != m_storage.end());
    }

    Result<void> clear() override {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_storage.clear();
        return Result<void>::Ok();
    }

private:
    mutable std::mutex m_mutex;
    std::map<std::string, std::string> m_storage;
};

std::unique_ptr<ISecureStorage> makeMbedTLSSecureStorage() {
    return std::make_unique<MbedTLSSecureStorage>();
}

} }

#endif
