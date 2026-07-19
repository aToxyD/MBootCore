#include <sdk/APICompatibility.hpp>
#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>
#include <filesystem>

namespace fs = std::filesystem;

namespace mbootcore {
namespace sdk {

APICompatibilityChecker::APICompatibilityChecker() = default;

static std::string readAllHeaders(const std::string& rootPath) {
    std::stringstream all;
    if (!fs::exists(rootPath)) return {};
    if (fs::is_directory(rootPath)) {
        for (const auto& entry : fs::recursive_directory_iterator(rootPath)) {
            if (entry.path().extension() == ".hpp") {
                std::ifstream ifs(entry.path());
                if (ifs.is_open()) all << ifs.rdbuf() << "\n";
            }
        }
    } else {
        std::ifstream ifs(rootPath);
        if (ifs.is_open()) all << ifs.rdbuf() << "\n";
    }
    return all.str();
}

CompatibilityReport APICompatibilityChecker::compareVersions(
    const std::string& oldHeadersPath,
    const std::string& newHeadersPath) const
{
    CompatibilityReport report;

    auto oldContent = readAllHeaders(oldHeadersPath);
    auto newContent = readAllHeaders(newHeadersPath);
    auto oldClasses = parseClassNames(oldContent);
    auto newClasses = parseClassNames(newContent);

    for (const auto& cls : newClasses) {
        if (oldClasses.find(cls) == oldClasses.end()) {
            APIDifference diff;
            diff.className = cls;
            diff.memberName = cls;
            diff.type = APIDifference::ChangeType::Added;
            diff.breaking = false;
            report.differences.push_back(diff);
            report.addedCount++;
            report.nonBreakingChanges.push_back("Added class: " + cls);
        }
    }

    for (const auto& cls : oldClasses) {
        if (newClasses.find(cls) == newClasses.end()) {
            APIDifference diff;
            diff.className = cls;
            diff.memberName = cls;
            diff.type = APIDifference::ChangeType::Removed;
            diff.breaking = true;
            report.differences.push_back(diff);
            report.removedCount++;
            report.breakingChanges.push_back("Removed class: " + cls);
        }
    }

    report.compatible = report.removedCount == 0;
    report.summary = "Compared headers: " + oldHeadersPath + " -> " + newHeadersPath + ". "
                     "Added: " + std::to_string(report.addedCount) + ", "
                     "Removed: " + std::to_string(report.removedCount) + ", "
                     "Changed: " + std::to_string(report.changedCount) + ".";

    return report;
}

CompatibilityReport APICompatibilityChecker::checkSDKVersion(
    const std::string& currentSDKVersion,
    const std::string& minCompatibleVersion) const
{
    CompatibilityReport report;

    auto compareVersions = [](const std::string& a, const std::string& b) -> int {
        auto parse = [](const std::string& v) -> std::vector<int> {
            std::vector<int> parts;
            std::istringstream iss(v);
            std::string part;
            while (std::getline(iss, part, '.')) {
                try {
                    parts.push_back(std::stoi(part));
                } catch (...) {
                    parts.push_back(0);
                }
            }
            return parts;
        };
        auto va = parse(a);
        auto vb = parse(b);
        size_t maxLen = std::max(va.size(), vb.size());
        va.resize(maxLen, 0);
        vb.resize(maxLen, 0);
        for (size_t i = 0; i < maxLen; ++i) {
            if (va[i] < vb[i]) return -1;
            if (va[i] > vb[i]) return 1;
        }
        return 0;
    };

    int cmp = compareVersions(currentSDKVersion, minCompatibleVersion);
    report.compatible = (cmp >= 0);

    if (!report.compatible) {
        report.breakingChanges.push_back(
            "SDK version " + currentSDKVersion + " is below minimum " + minCompatibleVersion);
        report.summary = "SDK version check FAILED: "
                         "current=" + currentSDKVersion +
                         " min=" + minCompatibleVersion;
    } else {
        report.summary = "SDK version check PASSED: "
                         "current=" + currentSDKVersion +
                         " min=" + minCompatibleVersion;
    }

    return report;
}

bool APICompatibilityChecker::isBreakingChange(const APIDifference& diff) const noexcept
{
    switch (diff.type) {
        case APIDifference::ChangeType::Removed:
        case APIDifference::ChangeType::SignatureChanged:
        case APIDifference::ChangeType::VirtualTableChanged:
            return true;
        case APIDifference::ChangeType::Added:
        case APIDifference::ChangeType::AccessChanged:
        case APIDifference::ChangeType::TypeChanged:
        default:
            return false;
    }
}

std::string APICompatibilityChecker::generateReport(
    const CompatibilityReport& report,
    const std::string& oldVersion,
    const std::string& newVersion) const
{
    std::stringstream ss;
    ss << "=== API Compatibility Report ===\n";
    ss << "Comparing: " << oldVersion << " -> " << newVersion << "\n";
    ss << "Status: " << (report.compatible ? "COMPATIBLE" : "INCOMPATIBLE") << "\n\n";
    ss << report.summary << "\n\n";

    ss << "Breaking Changes (" << report.breakingChanges.size() << "):\n";
    for (const auto& bc : report.breakingChanges)
        ss << "  [BREAKING] " << bc << "\n";

    ss << "\nNon-Breaking Changes (" << report.nonBreakingChanges.size() << "):\n";
    for (const auto& nb : report.nonBreakingChanges)
        ss << "  [NON-BREAKING] " << nb << "\n";

    ss << "\nWarnings (" << report.warnings.size() << "):\n";
    for (const auto& w : report.warnings)
        ss << "  [WARN] " << w << "\n";

    ss << "\nDetailed Differences (" << report.differences.size() << "):\n";
    for (const auto& d : report.differences) {
        std::string changeType;
        switch (d.type) {
            case APIDifference::ChangeType::Removed: changeType = "Removed"; break;
            case APIDifference::ChangeType::Added: changeType = "Added"; break;
            case APIDifference::ChangeType::SignatureChanged: changeType = "SignatureChanged"; break;
            case APIDifference::ChangeType::AccessChanged: changeType = "AccessChanged"; break;
            case APIDifference::ChangeType::TypeChanged: changeType = "TypeChanged"; break;
            case APIDifference::ChangeType::VirtualTableChanged: changeType = "VirtualTableChanged"; break;
        }
        ss << "  [" << changeType << "] " << d.className
           << "::" << d.memberName
           << (d.breaking ? " (BREAKING)" : "") << "\n";
    }

    return ss.str();
}

std::unordered_set<std::string> APICompatibilityChecker::parseClassNames(
    const std::string& headerContent) const
{
    std::unordered_set<std::string> classes;
    std::istringstream iss(headerContent);
    std::string line;
    while (std::getline(iss, line)) {
        auto trimmed = line;
        trimmed.erase(0, trimmed.find_first_not_of(" \t"));
        if (trimmed.find("class ") == 0) {
            auto start = trimmed.find(' ') + 1;
            auto end = trimmed.find_first_of(" \t:{;");
            if (end != std::string::npos && end > start) {
                std::string className = trimmed.substr(start, end - start);
                if (!className.empty() && className.find("//") != 0) {
                    classes.insert(className);
                }
            }
        }
    }
    return classes;
}

std::unordered_set<std::string> APICompatibilityChecker::parseMethodSignatures(
    const std::string& classContent) const
{
    std::unordered_set<std::string> methods;
    std::istringstream iss(classContent);
    std::string line;
    while (std::getline(iss, line)) {
        auto trimmed = line;
        trimmed.erase(0, trimmed.find_first_not_of(" \t"));
        if (trimmed.find("virtual ") == 0 || trimmed.find("void ") == 0 ||
            trimmed.find("int ") == 0 || trimmed.find("bool ") == 0 ||
            trimmed.find("std::") == 0 || trimmed.find("auto ") == 0) {
            auto paren = trimmed.find('(');
            auto semi = trimmed.find(';');
            if (paren != std::string::npos) {
                auto sigEnd = (semi != std::string::npos) ? semi : trimmed.size();
                std::string sig = trimmed.substr(0, sigEnd);
                if (!sig.empty()) {
                    methods.insert(sig);
                }
            }
        }
    }
    return methods;
}

} // namespace sdk
} // namespace mbootcore
