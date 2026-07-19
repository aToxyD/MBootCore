#include <mbootcore/security/SecurityManager.hpp>
#include <mutex>
#include <algorithm>

namespace mbootcore { namespace security {

struct SecurityManager::Impl {
    SecurityPolicy policy;
    std::vector<std::string> trustedKeys;
    mutable std::mutex mtx;
};

SecurityManager::SecurityManager()
    : m_impl(std::make_unique<Impl>()) {}

SecurityManager::~SecurityManager() = default;
SecurityManager::SecurityManager(SecurityManager&&) noexcept = default;
SecurityManager& SecurityManager::operator=(SecurityManager&&) noexcept = default;

Result<void> SecurityManager::setPolicy(const SecurityPolicy& policy) {
    std::lock_guard<std::mutex> lock(m_impl->mtx);
    m_impl->policy = policy;
    return {};
}

Result<SecurityPolicy> SecurityManager::policy() const {
    std::lock_guard<std::mutex> lock(m_impl->mtx);
    return m_impl->policy;
}

Result<VerificationResult> SecurityManager::verifyPackageSignature(const std::string&) {
    return ErrorCode::NotSupported;
}

Result<VerificationResult> SecurityManager::verifyPluginSignature(const std::string&) {
    return ErrorCode::NotSupported;
}

Result<VerificationResult> SecurityManager::verifyDSPSignature(const std::string&) {
    return ErrorCode::NotSupported;
}

Result<SecureHash> SecurityManager::computeHash(const std::string&, HashAlgorithm) {
    return ErrorCode::NotSupported;
}

Result<bool> SecurityManager::isTampered(const std::string&) {
    return ErrorCode::NotSupported;
}

Result<void> SecurityManager::addTrustedKey(const std::string& key) {
    std::lock_guard<std::mutex> lock(m_impl->mtx);
    auto it = std::find(m_impl->trustedKeys.begin(), m_impl->trustedKeys.end(), key);
    if (it == m_impl->trustedKeys.end()) {
        m_impl->trustedKeys.push_back(key);
    }
    return {};
}

Result<void> SecurityManager::removeTrustedKey(const std::string& key) {
    std::lock_guard<std::mutex> lock(m_impl->mtx);
    auto it = std::find(m_impl->trustedKeys.begin(), m_impl->trustedKeys.end(), key);
    if (it != m_impl->trustedKeys.end()) {
        m_impl->trustedKeys.erase(it);
    }
    return {};
}

Result<std::vector<std::string>> SecurityManager::trustedKeys() const {
    std::lock_guard<std::mutex> lock(m_impl->mtx);
    return m_impl->trustedKeys;
}

SecurityLevel SecurityManager::securityLevel() const {
    std::lock_guard<std::mutex> lock(m_impl->mtx);
    return m_impl->policy.level;
}

} }
