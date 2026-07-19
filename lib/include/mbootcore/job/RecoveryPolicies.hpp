#pragma once

#include <mbootcore/domain/Error.hpp>

#include <string>
#include <memory>

namespace mbootcore {
namespace job {

enum class RecoveryAction : uint32_t {
    Retry   = 0,
    Rollback = 1,
    Skip    = 2,
    Abort   = 3
};

class RecoveryPolicy {
public:
    virtual ~RecoveryPolicy() = default;

    virtual RecoveryAction evaluate(const std::string& jobId,
                                     ErrorCode error,
                                     uint32_t attemptCount) = 0;

    virtual int maxRetries() const noexcept = 0;
    virtual int maxRollbacks() const noexcept = 0;
    virtual bool canRollback() const noexcept = 0;

    virtual std::unique_ptr<RecoveryPolicy> clone() const = 0;
};

class DefaultRecoveryPolicy : public RecoveryPolicy {
public:
    explicit DefaultRecoveryPolicy(int maxRetries = 3, int maxRollbacks = 1, bool canRollback = true)
        : m_maxRetries(maxRetries), m_maxRollbacks(maxRollbacks), m_canRollback(canRollback) {}

    RecoveryAction evaluate(const std::string&, ErrorCode error, uint32_t attemptCount) override;

    int maxRetries() const noexcept override { return m_maxRetries; }
    int maxRollbacks() const noexcept override { return m_maxRollbacks; }
    bool canRollback() const noexcept override { return m_canRollback; }

    std::unique_ptr<RecoveryPolicy> clone() const override {
        return std::make_unique<DefaultRecoveryPolicy>(*this);
    }

private:
    int m_maxRetries;
    int m_maxRollbacks;
    bool m_canRollback;
    int m_rollbackCount{0};
};

class RetryForeverPolicy : public RecoveryPolicy {
public:
    explicit RetryForeverPolicy(int maxRollbacks = 1)
        : m_maxRollbacks(maxRollbacks) {}

    RecoveryAction evaluate(const std::string&, ErrorCode, uint32_t) override {
        return RecoveryAction::Retry;
    }

    int maxRetries() const noexcept override { return 1000000; }
    int maxRollbacks() const noexcept override { return m_maxRollbacks; }
    bool canRollback() const noexcept override { return false; }

    std::unique_ptr<RecoveryPolicy> clone() const override {
        return std::make_unique<RetryForeverPolicy>(*this);
    }

private:
    int m_maxRollbacks;
};

class AbortOnFailurePolicy : public RecoveryPolicy {
public:
    RecoveryAction evaluate(const std::string&, ErrorCode, uint32_t) override {
        return RecoveryAction::Abort;
    }

    int maxRetries() const noexcept override { return 0; }
    int maxRollbacks() const noexcept override { return 0; }
    bool canRollback() const noexcept override { return false; }

    std::unique_ptr<RecoveryPolicy> clone() const override {
        return std::make_unique<AbortOnFailurePolicy>(*this);
    }
};

class RollbackOnFailurePolicy : public RecoveryPolicy {
public:
    explicit RollbackOnFailurePolicy(int maxRollbacks = 1)
        : m_maxRollbacks(maxRollbacks) {}

    RecoveryAction evaluate(const std::string&, ErrorCode, uint32_t attemptCount) override;

    int maxRetries() const noexcept override { return 0; }
    int maxRollbacks() const noexcept override { return m_maxRollbacks; }
    bool canRollback() const noexcept override { return true; }

    std::unique_ptr<RecoveryPolicy> clone() const override {
        return std::make_unique<RollbackOnFailurePolicy>(*this);
    }

private:
    int m_maxRollbacks;
    int m_rollbackCount{0};
};

} // namespace job
} // namespace mbootcore
