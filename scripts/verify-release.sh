#!/usr/bin/env bash
# =============================================================================
# MBootCore — Release Verification Script (Linux / macOS / Bash)
# verify-release.sh
#
# Verifies that MBootCore release packages are complete and functional.
# Extracts packages and checks contents, find_package(), and SHA-256 checksums.
#
# Usage:
#   ./scripts/verify-release.sh                              # verify dist/
#   ./scripts/verify-release.sh --package-dir dist/          # verify specific dir
#   ./scripts/verify-release.sh --install-dir /opt/MBootCore # verify install tree
#   ./scripts/verify-release.sh --skip-build                 # skip consumer build test
# =============================================================================

set -euo pipefail

PACKAGE_DIR=""
INSTALL_DIR=""
SKIP_BUILD_TEST=false

# ── Parse arguments ───────────────────────────────────────────────────────────
while [[ $# -gt 0 ]]; do
    case "$1" in
        --package-dir)  PACKAGE_DIR="$2";  shift 2 ;;
        --install-dir)  INSTALL_DIR="$2";  shift 2 ;;
        --skip-build)   SKIP_BUILD_TEST=true; shift ;;
        *) echo "Unknown argument: $1"; exit 1 ;;
    esac
done

# Default: look for dist/ relative to script location
if [[ -z "$PACKAGE_DIR" && -z "$INSTALL_DIR" ]]; then
    SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
    PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
    PACKAGE_DIR="$PROJECT_ROOT/dist"
fi

GREEN="\033[32m"
RED="\033[31m"
YELLOW="\033[33m"
RESET="\033[0m"

PASSED=0
FAILED=0
WARNED=0

check() {
    local label="$1"
    local condition="$2"
    local detail="${3:-}"

    if $condition; then
        echo -e "  ${GREEN}✓${RESET} $label"
        PASSED=$((PASSED+1))
    else
        if [[ -n "$detail" ]]; then
            echo -e "  ${RED}✗${RESET} $label: $detail"
        else
            echo -e "  ${RED}✗${RESET} $label"
        fi
        FAILED=$((FAILED+1))
    fi
}

warn() {
    echo -e "  ${YELLOW}⚠${RESET} $1"
    WARNED=$((WARNED+1))
}

section() {
    echo ""
    echo -e "${GREEN}── $1${RESET}"
}

# ── Verify from package directory ─────────────────────────────────────────────
if [[ -n "$PACKAGE_DIR" ]]; then
    echo ""
    echo -e "${GREEN}MBootCore Release Verification${RESET}"
    echo "  Package directory: $PACKAGE_DIR"
    echo "$(printf '%.0s-' {1..60})"

    # Check for packages
    section "Package Files"

    ZIP_FILE=$(ls -t "$PACKAGE_DIR"/*.zip 2>/dev/null | head -1)
    DEB_FILE=$(ls -t "$PACKAGE_DIR"/*.deb 2>/dev/null | head -1)

    FOUND_PKG=false

    if [[ -n "$ZIP_FILE" ]]; then
        check "ZIP package exists" true "$(basename "$ZIP_FILE")"
        FOUND_PKG=true
    else
        check "ZIP package exists" false "not found in $PACKAGE_DIR"
    fi

    if [[ -n "$DEB_FILE" ]]; then
        check "DEB package exists" true "$(basename "$DEB_FILE")"
    fi

    if [[ "$FOUND_PKG" == "false" ]]; then
        echo ""
        echo -e "${RED}No packages found in $PACKAGE_DIR${RESET}"
        exit 1
    fi

    # Verify ZIP contents
    section "ZIP Contents"

    if [[ -n "$ZIP_FILE" ]]; then
        VERIFY_TMP=$(mktemp -d -t mbootcore_verify_XXXXXXX)
        unzip -q "$ZIP_FILE" -d "$VERIFY_TMP" 2>/dev/null || true

        # Find root of extracted content
        VERIFY_ROOT="$VERIFY_TMP"
        SUBDIR_COUNT=$(find "$VERIFY_TMP" -mindepth 1 -maxdepth 1 -type d 2>/dev/null | wc -l)
        if [[ "$SUBDIR_COUNT" -eq 1 ]]; then
            VERIFY_ROOT=$(find "$VERIFY_TMP" -mindepth 1 -maxdepth 1 -type d | head -1)
        fi

        check "Contains include/"         test -d "$VERIFY_ROOT/include"
        check "Contains lib/"             test -d "$VERIFY_ROOT/lib"
        check "libmbootcore.a"            test -f "$VERIFY_ROOT/lib/libmbootcore.a"
        check "MBootCoreConfig.cmake"     test -f "$VERIFY_ROOT/lib/cmake/MBootCore/MBootCoreConfig.cmake"
        check "MBootCoreTargets.cmake"    test -f "$VERIFY_ROOT/lib/cmake/MBootCore/MBootCoreTargets.cmake"
        check "MBootCoreConfigVersion.cmake" test -f "$VERIFY_ROOT/lib/cmake/MBootCore/MBootCoreConfigVersion.cmake"

        # Check headers
        if [[ -d "$VERIFY_ROOT/include/mbootcore" ]]; then
            HEADER_COUNT=$(find "$VERIFY_ROOT/include/mbootcore" -name "*.hpp" 2>/dev/null | wc -l)
            if [[ $HEADER_COUNT -ge 200 ]]; then
                check "200+ public headers" true "Found: $HEADER_COUNT"
            else
                check "200+ public headers" false "Found: $HEADER_COUNT"
            fi
        else
            check "200+ public headers" false "include/mbootcore/ not found"
        fi

        # Check SDK headers
        if [[ -d "$VERIFY_ROOT/include/sdk" ]]; then
            SDK_COUNT=$(find "$VERIFY_ROOT/include/sdk" -name "*.hpp" 2>/dev/null | wc -l)
            if [[ $SDK_COUNT -ge 19 ]]; then
                check "19+ SDK headers" true "Found: $SDK_COUNT"
            else
                check "19+ SDK headers" false "Found: $SDK_COUNT"
            fi
        else
            check "19+ SDK headers" false "include/sdk/ not found"
        fi

        # Check documentation
        section "Documentation"

        if [[ -d "$VERIFY_ROOT/share/doc/MBootCore" ]]; then
            check "Documentation README" test -f "$VERIFY_ROOT/share/doc/MBootCore/README.md"
            check "getting-started/"     test -d "$VERIFY_ROOT/share/doc/MBootCore/getting-started"
            check "user-guide/"          test -d "$VERIFY_ROOT/share/doc/MBootCore/user-guide"
            check "architecture/"        test -d "$VERIFY_ROOT/share/doc/MBootCore/architecture"
            check "reference/"           test -d "$VERIFY_ROOT/share/doc/MBootCore/reference"
        else
            check "Documentation installed" false "share/doc/MBootCore/ not found"
        fi

        # CMake config content check
        section "CMake Config Content"

        CONFIG_FILE="$VERIFY_ROOT/lib/cmake/MBootCore/MBootCoreConfig.cmake"
        if [[ -f "$CONFIG_FILE" ]]; then
            if grep -q "MBootCore_VERSION" "$CONFIG_FILE"; then check "MBootCore_VERSION defined" true; else check "MBootCore_VERSION defined" false; fi
            if grep -q "MBootCore_FOUND" "$CONFIG_FILE"; then check "MBootCore_FOUND defined" true; else check "MBootCore_FOUND defined" false; fi
            if grep -q "MBootCoreTargets.cmake" "$CONFIG_FILE"; then check "MBootCoreTargets.cmake included" true; else check "MBootCoreTargets.cmake included" false; fi
            if grep -q "Desktop.MBootCore" "$CONFIG_FILE"; then check "No absolute source paths" false; else check "No absolute source paths" true; fi
        else
            check "MBootCoreConfig.cmake exists" false
        fi

        # Checksums
        section "Checksum Verification"

        SHA_FILE="$PACKAGE_DIR/SHA256SUMS.txt"
        if [[ -f "$SHA_FILE" ]]; then
            check "SHA256SUMS.txt exists" true
            if command -v sha256sum >/dev/null 2>&1; then
                if (cd "$PACKAGE_DIR" && sha256sum -c "$SHA_FILE" >/dev/null 2>&1); then
                    check "SHA-256 checksums valid" true
                else
                    check "SHA-256 checksums valid" false "checksum mismatch"
                fi
            else
                warn "sha256sum not found — skipping checksum verification"
            fi
        else
            warn "SHA256SUMS.txt not found"
        fi

        rm -rf "$VERIFY_TMP"

        # find_package() build test using extracted ZIP
        if [[ "$SKIP_BUILD_TEST" == "false" ]]; then
            section "find_package(MBootCore) Build Test"

            VERIFY_TMP2=$(mktemp -d -t mbootcore_verify_XXXXXXX)
            unzip -q "$ZIP_FILE" -d "$VERIFY_TMP2" 2>/dev/null || true
            VERIFY_ROOT2="$VERIFY_TMP2"
            SUBDIR_COUNT2=$(find "$VERIFY_TMP2" -mindepth 1 -maxdepth 1 -type d 2>/dev/null | wc -l)
            if [[ "$SUBDIR_COUNT2" -eq 1 ]]; then
                VERIFY_ROOT2=$(find "$VERIFY_TMP2" -mindepth 1 -maxdepth 1 -type d | head -1)
            fi

            cat > "$VERIFY_TMP2/CMakeLists.txt" << 'EOF'
cmake_minimum_required(VERSION 3.20)
project(MBootVerify CXX)
set(CMAKE_CXX_STANDARD 17)
find_package(MBootCore REQUIRED)
add_executable(verify_consumer main.cpp)
target_link_libraries(verify_consumer PRIVATE MBootCore::mbootcore)
EOF

            cat > "$VERIFY_TMP2/main.cpp" << 'EOF'
#include <mbootcore/domain/Error.hpp>
int main() {
    using namespace mbootcore;
    auto r = Result<int>::Ok(1);
    return r.isOk() ? 0 : 1;
}
EOF

            BUILD_TMP="$VERIFY_TMP2/build"
            mkdir -p "$BUILD_TMP"

            CMAKE_ARGS=("-B" "$BUILD_TMP" "-DMBootCore_DIR=$VERIFY_ROOT2/lib/cmake/MBootCore")

            if cmake "$VERIFY_TMP2" "${CMAKE_ARGS[@]}" >/dev/null 2>&1; then
                check "find_package CMake configure" true

                if cmake --build "$BUILD_TMP" -j4 >/dev/null 2>&1; then
                    check "Consumer executable builds" true

                    if [[ -x "$BUILD_TMP/verify_consumer" ]]; then
                        if "$BUILD_TMP/verify_consumer" >/dev/null 2>&1; then
                            check "Consumer executable runs (exit 0)" true
                        else
                            check "Consumer executable runs (exit 0)" false
                        fi
                    else
                        check "Consumer executable exists" false
                    fi
                else
                    check "Consumer executable builds" false
                fi
            else
                check "find_package CMake configure" false
            fi

            rm -rf "$VERIFY_TMP2"
        else
            warn "Build test skipped (--skip-build)"
        fi
    fi

# ── Verify from install directory (backward compatible) ──────────────────────
elif [[ -n "$INSTALL_DIR" ]]; then
    echo ""
    echo -e "${GREEN}MBootCore Release Verification${RESET}"
    echo "  Install directory: $INSTALL_DIR"
    echo "$(printf '%.0s-' {1..60})"

    section "Install Tree"

    declare -A CORE_CHECKS=(
        ["libmbootcore.a"]="lib/libmbootcore.a"
        ["MBootCoreConfig.cmake"]="lib/cmake/MBootCore/MBootCoreConfig.cmake"
        ["MBootCoreTargets.cmake"]="lib/cmake/MBootCore/MBootCoreTargets.cmake"
        ["MBootCoreConfigVersion.cmake"]="lib/cmake/MBootCore/MBootCoreConfigVersion.cmake"
        ["MBootCore.hpp (master include)"]="include/mbootcore/MBootCore.hpp"
        ["domain/Error.hpp"]="include/mbootcore/domain/Error.hpp"
        ["api/ApiReference.hpp"]="include/mbootcore/api/ApiReference.hpp"
        ["sdk/SDKInfo.hpp"]="include/sdk/SDKInfo.hpp"
        ["sdk/VendorSDK.hpp"]="include/sdk/VendorSDK.hpp"
        ["examples directory"]="share/doc/examples"
        ["docs directory"]="share/doc/MBootCore"
        ["templates directory"]="share/doc/templates"
        ["LICENSE"]="share/doc/MBootCore/LICENSE"
        ["CHANGELOG.md"]="share/doc/MBootCore/CHANGELOG.md"
    )

    for label in "${!CORE_CHECKS[@]}"; do
        path="$INSTALL_DIR/${CORE_CHECKS[$label]}"
        if [[ -e "$path" ]]; then
            check "$label" true
        else
            check "$label" false
        fi
    done

    HEADER_COUNT=$(find "$INSTALL_DIR/include/mbootcore" -name "*.hpp" 2>/dev/null | wc -l || echo 0)
    if [[ $HEADER_COUNT -ge 200 ]]; then
        check "200+ public headers present" true "Found: $HEADER_COUNT"
    else
        check "200+ public headers present" false "Found: $HEADER_COUNT"
    fi

    SDK_HEADER_COUNT=$(find "$INSTALL_DIR/include/sdk" -name "*.hpp" 2>/dev/null | wc -l || echo 0)
    if [[ $SDK_HEADER_COUNT -ge 19 ]]; then
        check "19 SDK headers present" true "Found: $SDK_HEADER_COUNT"
    else
        check "19 SDK headers present" false "Found: $SDK_HEADER_COUNT"
    fi

    EXAMPLE_COUNT=$(find "$INSTALL_DIR/share/doc/examples" -mindepth 1 -maxdepth 1 -type d 2>/dev/null | wc -l || echo 0)
    if [[ $EXAMPLE_COUNT -ge 14 ]]; then
        check "14+ examples installed" true "Found: $EXAMPLE_COUNT"
    else
        check "14+ examples installed" false "Found: $EXAMPLE_COUNT"
    fi

    section "CMake Config Content"

    CONFIG_FILE="$INSTALL_DIR/lib/cmake/MBootCore/MBootCoreConfig.cmake"
    if [[ -f "$CONFIG_FILE" ]]; then
        if grep -q "MBootCore_VERSION" "$CONFIG_FILE"; then check "MBootCore_VERSION defined" true; else check "MBootCore_VERSION defined" false; fi
        if grep -q "MBootCore_FOUND" "$CONFIG_FILE"; then check "MBootCore_FOUND defined" true; else check "MBootCore_FOUND defined" false; fi
        if grep -q "MBootCoreTargets.cmake" "$CONFIG_FILE"; then check "MBootCoreTargets.cmake included" true; else check "MBootCoreTargets.cmake included" false; fi
        if grep -q "Desktop.MBootCore" "$CONFIG_FILE"; then check "No absolute source paths" false; else check "No absolute source paths" true; fi
    else
        check "MBootCoreConfig.cmake exists" false
    fi

    VERSION_FILE="$INSTALL_DIR/lib/cmake/MBootCore/MBootCoreConfigVersion.cmake"
    if [[ -f "$VERSION_FILE" ]]; then
        if grep -q 'set(PACKAGE_VERSION "[0-9]\+\.[0-9]\+\.[0-9]\+")' "$VERSION_FILE"; then check "Package version in config" true; else check "Package version in config" false; fi
    fi

    if [[ "$SKIP_BUILD_TEST" == "false" ]]; then
        section "find_package(MBootCore) Build Test"

        TMP_DIR=$(mktemp -d -t mbootcore_verify_XXXXXXX)

        cat > "$TMP_DIR/CMakeLists.txt" << 'EOF'
cmake_minimum_required(VERSION 3.20)
project(MBootVerify CXX)
set(CMAKE_CXX_STANDARD 17)
find_package(MBootCore REQUIRED)
add_executable(verify_consumer main.cpp)
target_link_libraries(verify_consumer PRIVATE MBootCore::mbootcore)
EOF

        cat > "$TMP_DIR/main.cpp" << 'EOF'
#include <mbootcore/domain/Error.hpp>
int main() {
    using namespace mbootcore;
    auto r = Result<int>::Ok(1);
    return r.isOk() ? 0 : 1;
}
EOF

        BUILD_TMP="$TMP_DIR/build"
        mkdir -p "$BUILD_TMP"

        CMAKE_ARGS=("-B" "$BUILD_TMP" "-DMBootCore_DIR=$INSTALL_DIR/lib/cmake/MBootCore")

        if cmake "$TMP_DIR" "${CMAKE_ARGS[@]}" >/dev/null 2>&1; then
            check "find_package CMake configure" true

            if cmake --build "$BUILD_TMP" -j4 >/dev/null 2>&1; then
                check "Consumer executable builds" true

                if [[ -x "$BUILD_TMP/verify_consumer" ]]; then
                    if "$BUILD_TMP/verify_consumer" >/dev/null 2>&1; then
                        check "Consumer executable runs (exit 0)" true
                    else
                        check "Consumer executable runs (exit 0)" false
                    fi
                else
                    check "Consumer executable exists" false
                fi
            else
                check "Consumer executable builds" false
            fi
        else
            check "find_package CMake configure" false
        fi

        rm -rf "$TMP_DIR"
    else
        warn "Build test skipped (--skip-build)"
    fi
fi

# ── Summary ───────────────────────────────────────────────────────────────────
echo ""
echo "$(printf '═%.0s' {1..60})"
if [[ $FAILED -eq 0 ]]; then
    echo -e "${GREEN}VERIFICATION PASSED${RESET} — $PASSED checks, $WARNED warnings"
else
    echo -e "${RED}VERIFICATION FAILED${RESET} — $PASSED passed, $FAILED FAILED, $WARNED warnings"
    exit 1
fi
echo "$(printf '═%.0s' {1..60})"
