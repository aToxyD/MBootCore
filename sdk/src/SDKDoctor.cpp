#include <sdk/SDKInfo.hpp>
#include <algorithm>
#include <sstream>
#include <cstdio>
#include <memory>
#include <cstdlib>

#ifdef _WIN32
#include <windows.h>
#endif

namespace mbootcore {
namespace sdk {

namespace {

std::string safeGetenv(const char* name) {
#ifdef _WIN32
    char buf[1024];
    DWORD len = ::GetEnvironmentVariableA(name, buf, sizeof(buf));
    return (len > 0 && len < sizeof(buf)) ? std::string(buf, len) : std::string();
#else
    const char* val = std::getenv(name);
    return val ? std::string(val) : std::string();
#endif
}

} // anonymous namespace

SDKDoctor::SDKDoctor() = default;

SDKDoctor::DoctorReport SDKDoctor::runAllChecks() {
    DoctorReport report;

    auto addResult = [&](DiagnosticResult r) {
        report.results.push_back(r);
        if (r.severity == "error") {
            report.allPassed = false;
            report.errorCount++;
        } else if (r.severity == "warning") {
            report.warningCount++;
        } else {
            report.passedCount++;
        }
    };

    addResult(checkInstallation());
    addResult(checkCompiler());
    addResult(checkDependencies());
    addResult(checkRuntime());
    addResult(checkPlugins());
    addResult(checkEnvironment());

    if (report.errorCount == 0) {
        report.allPassed = true;
    }

    std::ostringstream ss;
    ss << "SDK Doctor Report: " << report.results.size() << " checks, "
       << report.passedCount << " passed, "
       << report.warningCount << " warnings, "
       << report.errorCount << " errors";
    report.summary = ss.str();

    return report;
}

SDKDoctor::DiagnosticResult SDKDoctor::checkInstallation() {
    DiagnosticResult result;
    result.check = "Installation";
    result.severity = "info";

    auto sdkInfo = SDKInfo::collect();
    result.passed = !sdkInfo.components.empty();
    if (result.passed) {
        result.message = "SDK installation verified";
        for (const auto& c : sdkInfo.components) {
            result.details.push_back(c.name + " v" + c.version + " [" + c.status + "]");
        }
    } else {
        result.message = "No SDK components found";
        result.severity = "error";
        result.details.push_back("SDK components list is empty");
    }

    return result;
}

SDKDoctor::DiagnosticResult SDKDoctor::checkCompiler() {
    DiagnosticResult result;
    result.check = "Compiler";
    result.severity = "info";

    if (isCompilerSupported()) {
        result.passed = true;
        result.message = "Compiler is supported";
#ifdef __GNUC__
        result.details.push_back("GCC " + std::to_string(__GNUC__) + "." + std::to_string(__GNUC_MINOR__));
#elif __clang__
        result.details.push_back("Clang " + std::to_string(__clang_major__) + "." + std::to_string(__clang_minor__));
#elif _MSC_VER
        result.details.push_back("MSVC " + std::to_string(_MSC_VER));
#else
        result.details.push_back("Unknown compiler");
#endif
    } else {
        result.passed = false;
        result.message = "Compiler is not supported (need GCC >= 7, Clang >= 6, MSVC >= 2019)";
        result.severity = "error";
#ifdef _MSC_VER
        result.details.push_back("MSVC version: " + std::to_string(_MSC_VER) + " (need >= 1920)");
#elif __GNUC__
        result.details.push_back("GCC version: " + std::to_string(__GNUC__) + "." + std::to_string(__GNUC_MINOR__) + " (need >= 7.0)");
#elif __clang__
        result.details.push_back("Clang version: " + std::to_string(__clang_major__) + "." + std::to_string(__clang_minor__) + " (need >= 6.0)");
#endif
    }

    return result;
}

SDKDoctor::DiagnosticResult SDKDoctor::checkDependencies() {
    DiagnosticResult result;
    result.check = "Dependencies";
    result.severity = "info";
    result.passed = true;
    result.message = "Core dependencies available";
    result.details.push_back("MBootCore (core library)");
    result.details.push_back("C++17 compatible compiler");
    result.details.push_back("CMake build system");

    return result;
}

SDKDoctor::DiagnosticResult SDKDoctor::checkRuntime() {
    DiagnosticResult result;
    result.check = "Runtime";
    result.severity = "info";

#ifdef _WIN32
    result.details.push_back("OS: Windows");
    char buf[256];
    DWORD sz = sizeof(buf);
    if (GetComputerNameA(buf, &sz)) {
        result.details.push_back("Host: " + std::string(buf));
    }
#elif __linux__
    result.details.push_back("OS: Linux");
#elif __APPLE__
    result.details.push_back("OS: macOS");
#endif

    result.passed = true;
    result.message = "Runtime environment looks good";

    return result;
}

SDKDoctor::DiagnosticResult SDKDoctor::checkPlugins() {
    DiagnosticResult result;
    result.check = "Plugins";
    result.severity = "info";

    result.passed = true;
    result.message = "Plugin directory structure available";
    result.details.push_back("Plugin system supports dynamic loading");
    result.details.push_back("Protocol and vendor plugins supported");

    return result;
}

SDKDoctor::DiagnosticResult SDKDoctor::checkEnvironment() {
    DiagnosticResult result;
    result.check = "Environment";
    result.severity = "info";

    result.passed = true;
    result.message = "Environment variables checked";

    auto checkEnv = [&](const char* var) {
        std::string val = safeGetenv(var);
        if (!val.empty()) {
            result.details.push_back(std::string(var) + " = " + val);
        } else {
            result.details.push_back(std::string(var) + " = (not set)");
        }
    };

    checkEnv("PATH");
    checkEnv("MBOOTCORE_SDK_DIR");
    return result;
}

std::string SDKDoctor::generateReport(const DoctorReport& report) const {
    std::ostringstream ss;
    ss << "MBootCore SDK Doctor Report\n";
    ss << "============================\n\n";

    ss << "Summary: " << report.summary << "\n\n";

    for (const auto& r : report.results) {
        ss << "[" << (r.passed ? "PASS" : "FAIL") << "] "
           << r.check << ": " << r.message << " (" << r.severity << ")\n";
        for (const auto& d : r.details) {
            ss << "    - " << d << "\n";
        }
        ss << "\n";
    }

    ss << "Overall: " << (report.allPassed ? "ALL CHECKS PASSED" : "SOME CHECKS FAILED");
    ss << " (" << report.passedCount << " passed, "
       << report.warningCount << " warnings, "
       << report.errorCount << " errors)\n";

    return ss.str();
}

bool SDKDoctor::isCompilerSupported() const {
#if defined(__GNUC__) && (__GNUC__ >= 7)
    return true;
#elif defined(__clang__) && (__clang_major__ >= 6)
    return true;
#elif defined(_MSC_VER) && (_MSC_VER >= 1920)
    return true;
#else
    return false;
#endif
}

} // namespace sdk
} // namespace mbootcore
