#pragma once

#include <memory>
#include <string>
#include <vector>

#include <mbootcore/security/SecurityTypes.hpp>
#include <mbootcore/domain/Error.hpp>

namespace mbootcore { namespace security {

class SecurityManager {
public:
    SecurityManager();
    ~SecurityManager();
    SecurityManager(const SecurityManager&) = delete;
    SecurityManager& operator=(const SecurityManager&) = delete;
    SecurityManager(SecurityManager&&) noexcept;
    SecurityManager& operator=(SecurityManager&&) noexcept;

    Result<void> setPolicy(const SecurityPolicy& policy);
    Result<SecurityPolicy> policy() const;

    Result<VerificationResult> verifyPackageSignature(const std::string& packagePath);
    Result<VerificationResult> verifyPluginSignature(const std::string& pluginPath);
    Result<VerificationResult> verifyDSPSignature(const std::string& dspPath);
    Result<SecureHash> computeHash(const std::string& data, HashAlgorithm algo);

    Result<bool> isTampered(const std::string& path);
    Result<void> addTrustedKey(const std::string& key);
    Result<void> removeTrustedKey(const std::string& key);
    Result<std::vector<std::string>> trustedKeys() const;

    SecurityLevel securityLevel() const;

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

} }
