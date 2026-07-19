#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <map>
#include <chrono>

namespace mbootcore { namespace security {

enum class HashAlgorithm : uint32_t { SHA256=0, SHA512=1 };
enum class SignatureAlgorithm : uint32_t { RSA2048=0, RSA4096=1, None=2 };
enum class SecurityLevel : uint32_t { None=0, Basic=1, Standard=2, High=3, FIPS=4 };
enum class VerificationResult : uint32_t { Verified=0, Failed=1, NotSigned=2, Unknown=3, Tampered=4 };

struct SecurityPolicy {
    SecurityLevel level{SecurityLevel::None};
    bool requirePackageSignatures{false};
    bool requirePluginSignatures{false};
    bool requireDSPSignatures{false};
    bool tamperDetection{false};
    HashAlgorithm hashAlgorithm{HashAlgorithm::SHA256};
    SignatureAlgorithm signatureAlgorithm{SignatureAlgorithm::None};
    std::vector<std::string> trustedKeys;
};

struct SignatureInfo {
    std::string packageId;
    std::string signer;
    std::string hashValue;
    SignatureAlgorithm algorithm{SignatureAlgorithm::None};
    std::chrono::system_clock::time_point timestamp;
    bool valid{false};
};

struct SecureHash {
    std::string value;
    HashAlgorithm algorithm{HashAlgorithm::SHA256};

    std::string toString() const;
    bool operator==(const SecureHash& o) const noexcept;
    bool operator!=(const SecureHash& o) const noexcept;
};

class SecurityManager;
class PermissionSet;
class SecureStorage;

} }
