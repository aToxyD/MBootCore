#pragma once

#include <string>

#include <mbootcore/domain/Error.hpp>

namespace mbootcore { namespace security {

class ISecureStorage {
public:
    virtual ~ISecureStorage() = default;
    virtual Result<void> store(const std::string& key, const std::string& value) = 0;
    virtual Result<std::string> retrieve(const std::string& key) = 0;
    virtual Result<void> remove(const std::string& key) = 0;
    virtual Result<bool> exists(const std::string& key) const = 0;
    virtual Result<void> clear() = 0;
};

class NotSupportedSecureStorage final : public ISecureStorage {
public:
    Result<void> store(const std::string& /*key*/, const std::string& /*value*/) override {
        return Result<void>::Error(ErrorCode::NotSupported);
    }
    Result<std::string> retrieve(const std::string& /*key*/) override {
        return Result<std::string>::Error(ErrorCode::NotSupported);
    }
    Result<void> remove(const std::string& /*key*/) override {
        return Result<void>::Error(ErrorCode::NotSupported);
    }
    Result<bool> exists(const std::string& /*key*/) const override {
        return Result<bool>::Error(ErrorCode::NotSupported);
    }
    Result<void> clear() override {
        return Result<void>::Error(ErrorCode::NotSupported);
    }
};

} }
