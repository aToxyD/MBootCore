#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>

namespace mbootcore {
namespace sdk {

struct APIDifference {
    std::string className;
    std::string memberName;
    enum class ChangeType {
        Removed,
        Added,
        SignatureChanged,
        AccessChanged,
        TypeChanged,
        VirtualTableChanged
    } type{ChangeType::Removed};
    std::string oldSignature;
    std::string newSignature;
    bool breaking{true};
};

struct CompatibilityReport {
    std::vector<APIDifference> differences;
    std::vector<std::string> breakingChanges;
    std::vector<std::string> nonBreakingChanges;
    std::vector<std::string> warnings;
    bool compatible{true};
    int addedCount{0};
    int removedCount{0};
    int changedCount{0};
    std::string summary;
};

class APICompatibilityChecker {
public:
    APICompatibilityChecker();

    CompatibilityReport compareVersions(const std::string& oldHeadersPath,
                                         const std::string& newHeadersPath) const;
    
    CompatibilityReport checkSDKVersion(const std::string& currentSDKVersion,
                                         const std::string& minCompatibleVersion) const;
    
    bool isBreakingChange(const APIDifference& diff) const noexcept;
    
    std::string generateReport(const CompatibilityReport& report,
                                const std::string& oldVersion,
                                const std::string& newVersion) const;

private:
    std::unordered_set<std::string> parseClassNames(const std::string& headerContent) const;
    std::unordered_set<std::string> parseMethodSignatures(const std::string& classContent) const;
};

} // namespace sdk
} // namespace mbootcore
