#pragma once

#include <string>
#include <vector>
#include <unordered_map>

namespace mbootcore {
namespace sdk {

struct HeaderIssue {
    std::string file;
    std::string issue;
    enum class Severity { Info, Warn, Error } severity{Severity::Warn};
};

struct IncludeIssue {
    std::string file;
    std::string include;
    std::string issue;
};

struct ABIReport {
    bool compatible{true};
    std::vector<std::string> changes;
    std::vector<std::string> warnings;
};

struct PublicAPIReport {
    std::vector<std::string> publicClasses;
    std::vector<std::string> publicInterfaces;
    std::vector<std::string> publicEnums;
    std::vector<std::string> publicFunctions;
    std::vector<std::string> missingHeaders;
    std::vector<std::string> missingIncludes;
    std::vector<HeaderIssue> headerIssues;
    bool valid{false};
    std::string summary;
};

class SDKValidator {
public:
    SDKValidator();

    PublicAPIReport validatePublicAPI(const std::vector<std::string>& headerRoots) const;
    PublicAPIReport validateSDKHeaders(const std::vector<std::string>& headerPaths) const;
    
    std::vector<HeaderIssue> checkHeaderSelfContainment(const std::string& headerPath) const;
    std::vector<HeaderIssue> checkIncludeOrdering(const std::string& headerPath) const;
    std::vector<HeaderIssue> checkNamespaceConsistency(const std::string& headerPath) const;
    
    ABIReport compareABI(const std::string& oldVersion, const std::string& newVersion) const;
    
    bool verifyNoPrivateHeadersLeaked(const std::vector<std::string>& publicHeaders,
                                       const std::vector<std::string>& privateHeaders) const;
    bool verifyForwardDeclarations(const std::vector<std::string>& headers) const;
    
    std::string generateReport(const PublicAPIReport& report) const;

private:
    bool isHeaderSelfContained(const std::string& content) const;
    bool hasValidNamespace(const std::string& content) const;
};

} // namespace sdk
} // namespace mbootcore
