#pragma once

#include <cstdint>
#include <string>

#include <mbootcore/domain/Error.hpp>

namespace mbootcore { namespace security {

class IIntegrityVerifier {
public:
    virtual ~IIntegrityVerifier() = default;
    virtual Result<bool> isTampered(const std::string& path) = 0;
    virtual Result<bool> verifyIntegrity(const std::string& path) = 0;
};

class NotSupportedIntegrityVerifier final : public IIntegrityVerifier {
public:
    Result<bool> isTampered(const std::string& /*path*/) override {
        return Result<bool>::Error(ErrorCode::NotSupported);
    }
    Result<bool> verifyIntegrity(const std::string& /*path*/) override {
        return Result<bool>::Error(ErrorCode::NotSupported);
    }
};

} }
