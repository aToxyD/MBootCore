#pragma once

#include <string>
#include <mbootcore/diagnostics/DiagnosticsTypes.hpp>

namespace mbootcore { namespace diagnostics {

class DiagnosticCheck {
public:
    virtual ~DiagnosticCheck() = default;
    virtual std::string id() const = 0;
    virtual std::string name() const = 0;
    virtual DiagnosticCategory category() const = 0;
    virtual Result<DiagnosticIssue> execute() = 0;
};

class RuntimeHealthCheck : public DiagnosticCheck {
public:
    std::string id() const override;
    std::string name() const override;
    DiagnosticCategory category() const override;
    Result<DiagnosticIssue> execute() override;
};

class MemoryHealthCheck : public DiagnosticCheck {
public:
    std::string id() const override;
    std::string name() const override;
    DiagnosticCategory category() const override;
    Result<DiagnosticIssue> execute() override;
};

class TransportHealthCheck : public DiagnosticCheck {
public:
    std::string id() const override;
    std::string name() const override;
    DiagnosticCategory category() const override;
    Result<DiagnosticIssue> execute() override;
};

class PipelineHealthCheck : public DiagnosticCheck {
public:
    std::string id() const override;
    std::string name() const override;
    DiagnosticCategory category() const override;
    Result<DiagnosticIssue> execute() override;
};

class PluginHealthCheck : public DiagnosticCheck {
public:
    std::string id() const override;
    std::string name() const override;
    DiagnosticCategory category() const override;
    Result<DiagnosticIssue> execute() override;
};

class DSPHealthCheck : public DiagnosticCheck {
public:
    std::string id() const override;
    std::string name() const override;
    DiagnosticCategory category() const override;
    Result<DiagnosticIssue> execute() override;
};

class ConfigurationHealthCheck : public DiagnosticCheck {
public:
    std::string id() const override;
    std::string name() const override;
    DiagnosticCategory category() const override;
    Result<DiagnosticIssue> execute() override;
};

class DeadlockDetectionCheck : public DiagnosticCheck {
public:
    std::string id() const override;
    std::string name() const override;
    DiagnosticCategory category() const override;
    Result<DiagnosticIssue> execute() override;
};

} }
