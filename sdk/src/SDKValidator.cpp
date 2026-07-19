#include <sdk/SDKValidator.hpp>
#include <algorithm>
#include <cctype>
#include <fstream>
#include <regex>
#include <sstream>
#include <filesystem>

namespace fs = std::filesystem;

namespace mbootcore {
namespace sdk {

SDKValidator::SDKValidator() = default;

PublicAPIReport SDKValidator::validatePublicAPI(const std::vector<std::string>& headerRoots) const
{
    PublicAPIReport report;

    for (const auto& root : headerRoots) {
        if (!fs::exists(root)) {
            report.missingHeaders.push_back(root);
            continue;
        }

        if (fs::is_directory(root)) {
            for (const auto& entry : fs::recursive_directory_iterator(root)) {
                if (entry.path().extension() == ".hpp") {
                    auto pathStr = entry.path().string();
                    std::ifstream ifs(pathStr);
                    if (!ifs.is_open()) continue;
                    std::stringstream buf;
                    buf << ifs.rdbuf();
                    std::string content = buf.str();

                    if (content.find("class ") != std::string::npos)
                        report.publicClasses.push_back(pathStr);
                    if (content.find("struct ") != std::string::npos &&
                        content.find("struct ") == content.find("struct ") && 
                        content.find(";") == std::string::npos)
                        report.publicInterfaces.push_back(pathStr);
                    if (content.find("enum class ") != std::string::npos ||
                        content.find("enum ") != std::string::npos)
                        report.publicEnums.push_back(pathStr);
                    if (content.find("virtual ") != std::string::npos ||
                        content.find("void ") != std::string::npos)
                        report.publicFunctions.push_back(pathStr);
                }
            }
        } else {
            std::ifstream ifs(root);
            if (!ifs.is_open()) continue;
            std::stringstream buf;
            buf << ifs.rdbuf();
            std::string content = buf.str();

            if (content.find("class ") != std::string::npos)
                report.publicClasses.push_back(root);
            if (content.find("enum class ") != std::string::npos ||
                content.find("enum ") != std::string::npos)
                report.publicEnums.push_back(root);
            if (content.find("virtual ") != std::string::npos)
                report.publicFunctions.push_back(root);
        }
    }

    report.valid = true;
    report.summary = "Validated " + std::to_string(headerRoots.size()) + " header roots. "
                     "Found " + std::to_string(report.publicClasses.size()) + " classes, "
                     + std::to_string(report.publicEnums.size()) + " enums, "
                     + std::to_string(report.publicFunctions.size()) + " functions.";
    return report;
}

PublicAPIReport SDKValidator::validateSDKHeaders(const std::vector<std::string>& headerPaths) const
{
    PublicAPIReport report;

    for (const auto& path : headerPaths) {
        if (!fs::exists(path)) {
            report.missingHeaders.push_back(path);
            continue;
        }

        std::ifstream ifs(path);
        if (!ifs.is_open()) {
            report.missingHeaders.push_back(path);
            continue;
        }
        std::stringstream buf;
        buf << ifs.rdbuf();
        std::string content = buf.str();

        if (!isHeaderSelfContained(content)) {
            HeaderIssue issue;
            issue.file = path;
            issue.issue = "Header is not self-contained (missing includes)";
            issue.severity = HeaderIssue::Severity::Error;
            report.headerIssues.push_back(issue);
        }

        if (!hasValidNamespace(content)) {
            HeaderIssue issue;
            issue.file = path;
            issue.issue = "Declaration outside mbootcore:: namespace";
            issue.severity = HeaderIssue::Severity::Warn;
            report.headerIssues.push_back(issue);
        }

        if (content.find("class ") != std::string::npos)
            report.publicClasses.push_back(path);
        if (content.find("enum class ") != std::string::npos ||
            content.find("enum ") != std::string::npos)
            report.publicEnums.push_back(path);
        if (content.find("virtual ") != std::string::npos)
            report.publicFunctions.push_back(path);
    }

    report.valid = report.missingHeaders.empty();
    report.summary = "Validated " + std::to_string(headerPaths.size()) + " SDK headers. "
                     "Issues found: " + std::to_string(report.headerIssues.size()) + ".";
    return report;
}

std::vector<HeaderIssue> SDKValidator::checkHeaderSelfContainment(const std::string& headerPath) const
{
    std::vector<HeaderIssue> issues;

    if (!fs::exists(headerPath)) {
        HeaderIssue issue;
        issue.file = headerPath;
        issue.issue = "Header file does not exist";
        issue.severity = HeaderIssue::Severity::Error;
        issues.push_back(issue);
        return issues;
    }

    std::ifstream ifs(headerPath);
    if (!ifs.is_open()) {
        HeaderIssue issue;
        issue.file = headerPath;
        issue.issue = "Cannot open header file";
        issue.severity = HeaderIssue::Severity::Error;
        issues.push_back(issue);
        return issues;
    }

    std::stringstream buf;
    buf << ifs.rdbuf();
    std::string content = buf.str();

    if (!isHeaderSelfContained(content)) {
        HeaderIssue issue;
        issue.file = headerPath;
        issue.issue = "Header is not self-contained - missing #include for types used in the header";
        issue.severity = HeaderIssue::Severity::Error;
        issues.push_back(issue);
    }

    bool hasIncludeGuard = content.find("#pragma once") != std::string::npos ||
                           content.find("#ifndef") != std::string::npos;
    if (!hasIncludeGuard) {
        HeaderIssue issue;
        issue.file = headerPath;
        issue.issue = "Missing include guard (#pragma once or #ifndef)";
        issue.severity = HeaderIssue::Severity::Warn;
        issues.push_back(issue);
    }

    return issues;
}

std::vector<HeaderIssue> SDKValidator::checkIncludeOrdering(const std::string& headerPath) const
{
    std::vector<HeaderIssue> issues;

    if (!fs::exists(headerPath)) {
        HeaderIssue issue;
        issue.file = headerPath;
        issue.issue = "Header file does not exist";
        issue.severity = HeaderIssue::Severity::Error;
        issues.push_back(issue);
        return issues;
    }

    std::ifstream ifs(headerPath);
    if (!ifs.is_open()) return issues;

    std::string line;
    bool inProjectInclude = false;
    int lineNum = 0;
    int lastStdLine = 0;

    while (std::getline(ifs, line)) {
        ++lineNum;
        auto trimmed = line;
        trimmed.erase(0, trimmed.find_first_not_of(" \t"));
        if (trimmed.empty() || trimmed[0] != '#') continue;
        if (trimmed.find("#include") != 0) continue;

        bool isStd = trimmed.find('<') != std::string::npos;
        bool isProject = trimmed.find('"') != std::string::npos;

        if (isStd) {
            if (inProjectInclude && lastStdLine == 0) {
                HeaderIssue issue;
                issue.file = headerPath;
                issue.issue = "Standard library include after project include on line " + std::to_string(lineNum);
                issue.severity = HeaderIssue::Severity::Warn;
                issues.push_back(issue);
            }
            lastStdLine = lineNum;
        } else if (isProject) {
            inProjectInclude = true;
        }
    }

    return issues;
}

std::vector<HeaderIssue> SDKValidator::checkNamespaceConsistency(const std::string& headerPath) const
{
    std::vector<HeaderIssue> issues;

    if (!fs::exists(headerPath)) {
        HeaderIssue issue;
        issue.file = headerPath;
        issue.issue = "Header file does not exist";
        issue.severity = HeaderIssue::Severity::Error;
        issues.push_back(issue);
        return issues;
    }

    std::ifstream ifs(headerPath);
    if (!ifs.is_open()) return issues;

    std::stringstream buf;
    buf << ifs.rdbuf();
    std::string content = buf.str();

    if (!hasValidNamespace(content)) {
        HeaderIssue issue;
        issue.file = headerPath;
        issue.issue = "Content is not properly wrapped in mbootcore:: namespace";
        issue.severity = HeaderIssue::Severity::Error;
        issues.push_back(issue);
    }

    std::istringstream iss(content);
    std::string line;
    int lineNum = 0;
    while (std::getline(iss, line)) {
        ++lineNum;
        auto trimmed = line;
        trimmed.erase(0, trimmed.find_first_not_of(" \t"));

        if (trimmed.find("class ") == 0 || trimmed.find("struct ") == 0 ||
            trimmed.find("enum ") == 0 || trimmed.find("using ") == 0) {
            if (content.find("namespace mbootcore") == std::string::npos) {
                HeaderIssue issue;
                issue.file = headerPath;
                issue.issue = "Declaration at line " + std::to_string(lineNum) + " is outside any namespace";
                issue.severity = HeaderIssue::Severity::Warn;
                issues.push_back(issue);
            }
        }
    }

    return issues;
}

ABIReport SDKValidator::compareABI(const std::string& oldVersion, const std::string& newVersion) const
{
    ABIReport report;
    report.compatible = false;
    report.warnings.push_back("ABI binary comparison requires compiler-specific tooling (e.g. abi-compliance-checker)");
    report.changes.push_back(
        "Binary ABI comparison between " + oldVersion + " and " + newVersion +
        " is not supported without external binary analysis tools");
    return report;
}

bool SDKValidator::verifyNoPrivateHeadersLeaked(
    const std::vector<std::string>& publicHeaders,
    const std::vector<std::string>& privateHeaders) const
{
    for (const auto& pub : publicHeaders) {
        std::string pubName = fs::path(pub).filename().string();
        for (const auto& priv : privateHeaders) {
            std::string privName = fs::path(priv).filename().string();
            if (pubName == privName) {
                return false;
            }
        }
    }
    return true;
}

bool SDKValidator::verifyForwardDeclarations(const std::vector<std::string>& headers) const
{
    for (const auto& h : headers) {
        if (!fs::exists(h)) continue;
        std::ifstream ifs(h);
        if (!ifs.is_open()) continue;
        std::stringstream buf;
        buf << ifs.rdbuf();
        std::string content = buf.str();

        size_t pos = 0;
        while ((pos = content.find("class ", pos)) != std::string::npos) {
            size_t semi = content.find(';', pos);
            size_t brace = content.find('{', pos);
            if (semi != std::string::npos && (brace == std::string::npos || semi < brace)) {
                pos = semi + 1;
                continue;
            }
            if (brace != std::string::npos) {
                size_t lineStart = content.rfind('\n', pos);
                if (lineStart == std::string::npos) lineStart = 0;
                std::string line = content.substr(lineStart, pos - lineStart);
                if (line.find("//") == std::string::npos && line.find("/*") == std::string::npos) {
                    size_t endClass = content.find('\n', brace);
                    std::string classDef = content.substr(pos, endClass - pos);
                    if (classDef.find("final") == std::string::npos &&
                        classDef.find("Q_OBJECT") == std::string::npos) {
                        return false;
                    }
                }
            }
            pos = brace != std::string::npos ? brace + 1 : pos + 1;
        }
    }
    return true;
}

std::string SDKValidator::generateReport(const PublicAPIReport& report) const
{
    std::stringstream ss;
    ss << "=== SDK Public API Report ===\n";
    ss << "Status: " << (report.valid ? "VALID" : "INVALID") << "\n";
    ss << report.summary << "\n\n";

    ss << "Public Classes (" << report.publicClasses.size() << "):\n";
    for (const auto& c : report.publicClasses)
        ss << "  " << c << "\n";

    ss << "Public Interfaces (" << report.publicInterfaces.size() << "):\n";
    for (const auto& i : report.publicInterfaces)
        ss << "  " << i << "\n";

    ss << "Public Enums (" << report.publicEnums.size() << "):\n";
    for (const auto& e : report.publicEnums)
        ss << "  " << e << "\n";

    ss << "Public Functions (" << report.publicFunctions.size() << "):\n";
    for (const auto& f : report.publicFunctions)
        ss << "  " << f << "\n";

    ss << "\nHeader Issues (" << report.headerIssues.size() << "):\n";
    for (const auto& hi : report.headerIssues) {
        std::string sev;
        switch (hi.severity) {
            case HeaderIssue::Severity::Info: sev = "Info"; break;
            case HeaderIssue::Severity::Warn: sev = "Warning"; break;
            case HeaderIssue::Severity::Error: sev = "Error"; break;
        }
        ss << "  [" << sev << "] " << hi.file << ": " << hi.issue << "\n";
    }

    ss << "\nMissing Headers (" << report.missingHeaders.size() << "):\n";
    for (const auto& mh : report.missingHeaders)
        ss << "  " << mh << "\n";

    return ss.str();
}

bool SDKValidator::isHeaderSelfContained(const std::string& content) const
{
    if (content.find("#include") == std::string::npos &&
        content.find("class ") == std::string::npos &&
        content.find("struct ") == std::string::npos) {
        return true;
    }
    if (content.find("std::") != std::string::npos &&
        content.find("#include <") == std::string::npos &&
        content.find("#include \"") == std::string::npos) {
        return false;
    }
    return true;
}

bool SDKValidator::hasValidNamespace(const std::string& content) const
{
    return content.find("namespace mbootcore") != std::string::npos;
}

} // namespace sdk
} // namespace mbootcore
