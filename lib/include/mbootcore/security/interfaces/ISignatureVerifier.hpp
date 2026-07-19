#pragma once

#include <cstdint>
#include <vector>
#include <string>

#include <mbootcore/domain/Error.hpp>

namespace mbootcore { namespace security {

enum class SignatureVerificationResult : uint32_t {
    Verified = 0,
    Failed = 1,
    NotSigned = 2,
    NotSupported = 3
};

class ISignatureVerifier {
public:
    virtual ~ISignatureVerifier() = default;
    virtual Result<SignatureVerificationResult> verify(const std::vector<uint8_t>& data, const std::vector<uint8_t>& signature) = 0;
    virtual Result<SignatureVerificationResult> verifyFile(const std::string& filePath) = 0;
};

class NotSupportedSignatureVerifier final : public ISignatureVerifier {
public:
    Result<SignatureVerificationResult> verify(const std::vector<uint8_t>& /*data*/, const std::vector<uint8_t>& /*signature*/) override {
        return Result<SignatureVerificationResult>::Ok(SignatureVerificationResult::NotSupported);
    }
    Result<SignatureVerificationResult> verifyFile(const std::string& /*filePath*/) override {
        return Result<SignatureVerificationResult>::Ok(SignatureVerificationResult::NotSupported);
    }
};

} }
