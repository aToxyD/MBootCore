#pragma once

#include <cstdint>
#include <vector>
#include <string>

#include <mbootcore/security/SecurityTypes.hpp>
#include <mbootcore/domain/Error.hpp>

namespace mbootcore { namespace security {

class IHashProvider {
public:
    virtual ~IHashProvider() = default;
    virtual Result<std::vector<uint8_t>> hash(const std::vector<uint8_t>& data, HashAlgorithm algo) const = 0;
};

class NotSupportedHashProvider final : public IHashProvider {
public:
    Result<std::vector<uint8_t>> hash(const std::vector<uint8_t>& /*data*/, HashAlgorithm /*algo*/) const override {
        return Result<std::vector<uint8_t>>::Error(ErrorCode::NotSupported);
    }
};

} }
