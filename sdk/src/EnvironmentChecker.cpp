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

// IMPORTANT:
// All commands passed to execCommand() must be compile-time string
// literals. Never construct shell commands from user input,
// configuration files, environment variables, network data, or file
// contents.
static std::string execCommand(const char* cmd) {
    std::string result;
#ifdef _WIN32
    std::unique_ptr<FILE, decltype(&_pclose)> pipe(_popen(cmd, "r"), _pclose);
#else
    std::unique_ptr<FILE, int(*)(FILE*)> pipe(popen(cmd, "r"), pclose);
#endif
    if (!pipe) return {};
    char buf[128];
    while (fgets(buf, sizeof(buf), pipe.get()) != nullptr) {
        result += buf;
    }
    while (!result.empty() && (result.back() == '\n' || result.back() == '\r')) {
        result.pop_back();
    }
    return result;
}

EnvironmentChecker::EnvironmentChecker() = default;

EnvironmentChecker::EnvironmentReport EnvironmentChecker::check() const {
    EnvironmentReport report;

#ifdef _WIN32
    report.osName = "Windows";
    OSVERSIONINFOA osvi;
    ZeroMemory(&osvi, sizeof(osvi));
    osvi.dwOSVersionInfoSize = sizeof(osvi);
    if (GetVersionExA(&osvi)) {
        report.osVersion = std::to_string(osvi.dwMajorVersion) + "." + std::to_string(osvi.dwMinorVersion);
    } else {
        report.osVersion = "unknown";
    }
    report.architecture = "x86_32";
#elif __linux__
    report.osName = "Linux";
    auto osRelease = execCommand("cat /etc/os-release 2>/dev/null | grep PRETTY_NAME | cut -d= -f2- | tr -d '\"'");
    report.osVersion = osRelease.empty() ? "unknown" : osRelease;
    auto arch = execCommand("uname -m");
    report.architecture = arch.empty() ? "unknown" : arch;
#elif __APPLE__
    report.osName = "macOS";
    auto swVer = execCommand("sw_vers -productVersion 2>/dev/null");
    report.osVersion = swVer.empty() ? "unknown" : swVer;
    auto arch = execCommand("uname -m");
    report.architecture = arch.empty() ? "unknown" : arch;
#else
    report.osName = "Unknown";
    report.osVersion = "unknown";
    report.architecture = "unknown";
#endif

    if (hasCMake()) {
        auto cmakeVer = execCommand("cmake --version");
        auto pos = cmakeVer.find("version");
        if (pos != std::string::npos) {
            report.cmakeVersion = cmakeVer.substr(pos + 8);
        } else {
            report.cmakeVersion = cmakeVer;
        }
    } else {
        report.missingDependencies.push_back("cmake");
    }

    if (!hasGit()) {
        report.missingDependencies.push_back("git");
    }

    if (!hasPython3()) {
        report.missingDependencies.push_back("python3");
    }

    if (hasCompiler()) {
#ifdef __GNUC__
        report.compilerName = "GCC";
        report.compilerVersion = std::to_string(__GNUC__) + "." + std::to_string(__GNUC_MINOR__);
#elif __clang__
        report.compilerName = "Clang";
        report.compilerVersion = std::to_string(__clang_major__) + "." + std::to_string(__clang_minor__);
#elif _MSC_VER
        report.compilerName = "MSVC";
        report.compilerVersion = std::to_string(_MSC_VER);
#else
        report.compilerName = "Unknown";
        report.compilerVersion = "unknown";
#endif
    } else {
        report.missingDependencies.push_back("compiler");
    }

    report.availablePaths = findSDKPaths();

    report.valid = report.missingDependencies.empty();

    return report;
}

bool EnvironmentChecker::hasCMake() const {
    auto out = execCommand("cmake --version");
    return !out.empty();
}

bool EnvironmentChecker::hasGit() const {
    auto out = execCommand("git --version");
    return !out.empty();
}

bool EnvironmentChecker::hasPython3() const {
    auto out = execCommand("python3 --version");
    return !out.empty();
}

bool EnvironmentChecker::hasCompiler() const {
#if defined(__GNUC__) || defined(__clang__) || defined(_MSC_VER)
    return true;
#else
    auto out = execCommand("g++ --version");
    if (!out.empty()) return true;
#ifdef _WIN32
    out = execCommand("cl --version");
    if (!out.empty()) return true;
#endif
    return false;
#endif
}

std::vector<std::string> EnvironmentChecker::findSDKPaths() const {
    std::vector<std::string> paths;

    const char* sdkDir = std::getenv("MBOOTCORE_SDK_DIR");
    if (sdkDir) {
        paths.push_back(sdkDir);
    }

#ifdef _WIN32
    const char* programFiles = std::getenv("ProgramFiles");
    if (programFiles) {
        paths.push_back(std::string(programFiles) + "\\MBootCore");
    }
    const char* localAppData = std::getenv("LOCALAPPDATA");
    if (localAppData) {
        paths.push_back(std::string(localAppData) + "\\MBootCore");
    }
#else
    paths.push_back("/usr/local/share/mbootcore");
    paths.push_back("/usr/share/mbootcore");
    paths.push_back("/opt/mbootcore");
#endif

    paths.push_back(".");

    return paths;
}

} // namespace sdk
} // namespace mbootcore
