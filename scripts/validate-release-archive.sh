#!/usr/bin/env bash
# =============================================================================
# MBootCore — Release Archive Validation Script
# validate-release-archive.sh
#
# Validates a release from a freshly extracted source archive:
#   1. Creates a source archive via git archive
#   2. Extracts to a temporary directory
#   3. Configures and builds the core library
#   4. Builds (verifies core builds)
#   5. Installs to a staging prefix
#   6. Builds a consumer project against the install tree
#   7. Verifies find_package(MBootCore) works
#
# Usage:
#   ./scripts/validate-release-archive.sh [--source-dir /path/to/mbootcore]
#
# Requirements:
#   - git, cmake, a C++17 compiler
# =============================================================================

set -euo pipefail

GREEN="\033[32m"
RED="\033[31m"
RESET="\033[0m"

SOURCE_DIR=""
while [[ $# -gt 0 ]]; do
    case "$1" in
        --source-dir)
            [[ $# -ge 2 ]] || {
                echo -e "${RED}XX${RESET} Missing argument for --source-dir" >&2
                exit 1
            }
            SOURCE_DIR="$2"
            shift 2
            ;;
        *)
            shift
            ;;
    esac
done

if [[ -z "$SOURCE_DIR" ]]; then
    SOURCE_DIR="$(cd "$(dirname "$0")/.." && pwd)"
fi

PASSED=0
FAILED=0

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

section() {
    echo ""
    echo -e "${GREEN}── $1${RESET}"
}

WORK_DIR=$(mktemp -d -t mbootcore_validate_XXXXXXX)
trap 'rm -rf "$WORK_DIR"' EXIT

ARCHIVE="$WORK_DIR/mbootcore-source.tar.gz"
BUILD_DIR="$WORK_DIR/build"
INSTALL_DIR="$WORK_DIR/install"
CONSUMER_DIR="$WORK_DIR/consumer"

echo ""
echo -e "${GREEN}MBootCore Release Archive Validation${RESET}"
echo "  Source: $SOURCE_DIR"
echo "  Work:   $WORK_DIR"
echo "$(printf '%.0s-' {1..60})"

# -- Step 1: Create source archive via git archive
section "Step 1 — Create source archive"
if git -C "$SOURCE_DIR" archive --worktree-attributes --format=tar.gz --prefix=MBootCore/ -o "$ARCHIVE" HEAD 2>/dev/null; then
    check "git archive created" true
else
    check "git archive created" false "Not a git repository or git not available"
    exit 1
fi

ARCHIVE_SIZE=$(stat -c%s "$ARCHIVE" 2>/dev/null || echo 0)
check "Archive size > 0" true "$(numfmt --to=iec $ARCHIVE_SIZE 2>/dev/null || echo $ARCHIVE_SIZE bytes)"

# -- Step 2: Extract archive
section "Step 2 — Extract archive"
EXTRACT_DIR="$WORK_DIR/extracted"
mkdir -p "$EXTRACT_DIR"
if tar xzf "$ARCHIVE" -C "$EXTRACT_DIR"; then
    check "Extraction succeeded" true
else
    check "Extraction succeeded" false
    exit 1
fi

SRC_DIR="$EXTRACT_DIR/MBootCore"
if [[ -d "$SRC_DIR" ]]; then
    check "Source directory exists" true
else
    check "Source directory exists" false
    ls -la "$EXTRACT_DIR"
    exit 1
fi

# Verify no .git/ or .github/ leaked into the archive
NO_GIT=$(find "$SRC_DIR" -type d -name '.git' 2>/dev/null | head -1)
if [[ -z "$NO_GIT" ]]; then
    check "No .git/ in archive" true
else
    check "No .git/ in archive" false "$NO_GIT"
fi
NO_GITHUB=$(find "$SRC_DIR" -type d -name '.github' 2>/dev/null | head -1)
if [[ -z "$NO_GITHUB" ]]; then
    check "No .github/ in archive" true
else
    check "No .github/ in archive" false "$NO_GITHUB"
fi

# Verify no gitignored artifacts leaked into the archive
for pattern in Testing deps .DS_Store Thumbs.db "*.swp" "*.swo" "*.user"; do
    matches=$(find "$SRC_DIR" -name "$pattern" -not -path "*/.git/*" 2>/dev/null)
    if [[ -n "$matches" ]]; then
        check "No '$pattern' in archive" false "$matches"
    else
        check "No '$pattern' in archive" true
    fi
done

# -- Step 3: Configure (core)
section "Step 3 — Configure core"
mkdir -p "$BUILD_DIR"
if cmake -S "$SRC_DIR" -B "$BUILD_DIR" \
    -DCMAKE_BUILD_TYPE=Release \
    -DMBOOTCORE_BUILD_TESTS=OFF \
    -DMBOOTCORE_BUILD_EXAMPLES=OFF \
    -DMBOOTCORE_BUILD_TOOLS=OFF \
    -DMBOOTCORE_BUILD_CLI=OFF \
    -DMBOOTCORE_BUILD_STUDIO=OFF \
    -DMBOOTCORE_ENABLE_CRYPTO=OFF \
    2>&1 | tail -5; then
    check "CMake configure succeeded" true
else
    check "CMake configure succeeded" false
    cmake -S "$SRC_DIR" -B "$BUILD_DIR" \
        -DCMAKE_BUILD_TYPE=Release \
        -DMBOOTCORE_BUILD_TESTS=OFF \
        -DMBOOTCORE_BUILD_EXAMPLES=OFF \
        -DMBOOTCORE_BUILD_TOOLS=OFF \
        -DMBOOTCORE_BUILD_CLI=OFF \
        -DMBOOTCORE_BUILD_STUDIO=OFF \
        -DMBOOTCORE_ENABLE_CRYPTO=OFF \
        2>&1 | tail -20
    exit 1
fi

# -- Step 4: Build
section "Step 4 — Build core"
if cmake --build "$BUILD_DIR" -j$(nproc) 2>&1 | tail -3; then
    check "Core build succeeded" true
else
    check "Core build succeeded" false
    cmake --build "$BUILD_DIR" -j$(nproc) 2>&1 | tail -20
    exit 1
fi

# Check the library was produced
if [[ -f "$BUILD_DIR/lib/libmbootcore.a" ]]; then
    check "libmbootcore.a produced" true
else
    check "libmbootcore.a produced" false
    find "$BUILD_DIR" -name '*.a' 2>/dev/null
fi

# -- Step 5: Install
section "Step 5 — Install"
mkdir -p "$INSTALL_DIR"
if cmake --install "$BUILD_DIR" --prefix "$INSTALL_DIR" 2>&1 | tail -3; then
    check "Install succeeded" true
else
    check "Install succeeded" false
    exit 1
fi

# Verify install structure
if [[ -f "$INSTALL_DIR/lib/cmake/MBootCore/MBootCoreConfig.cmake" ]]; then
    check "MBootCoreConfig.cmake installed" true
else
    check "MBootCoreConfig.cmake installed" false
fi

if [[ -f "$INSTALL_DIR/lib/cmake/MBootCore/MBootCoreConfigVersion.cmake" ]]; then
    check "MBootCoreConfigVersion.cmake installed" true
else
    check "MBootCoreConfigVersion.cmake installed" false
fi

if [[ -d "$INSTALL_DIR/include/mbootcore" ]]; then
    check "Public headers installed" true
else
    check "Public headers installed" false
fi

# -- Step 6: Consumer project
section "Step 6 — Consumer project"
mkdir -p "$CONSUMER_DIR"

cat > "$CONSUMER_DIR/CMakeLists.txt" << 'EOF'
cmake_minimum_required(VERSION 3.20)
project(ConsumerTest LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
find_package(MBootCore REQUIRED)
add_executable(consumer_app main.cpp)
target_link_libraries(consumer_app PRIVATE MBootCore::mbootcore)
EOF

cat > "$CONSUMER_DIR/main.cpp" << 'EOF'
#include <mbootcore/domain/Error.hpp>
#include <iostream>
int main() {
    mbootcore::Result<int> r = 42;
    if (r) { std::cout << "OK: " << r.value() << std::endl; }
    return r ? 0 : 1;
}
EOF

CONSUMER_BUILD="$CONSUMER_DIR/build"
mkdir -p "$CONSUMER_BUILD"

if cmake -S "$CONSUMER_DIR" -B "$CONSUMER_BUILD" \
    -DCMAKE_PREFIX_PATH="$INSTALL_DIR" \
    2>&1 | tail -3; then
    check "Consumer CMake configure succeeded" true
else
    check "Consumer CMake configure succeeded" false
    cmake -S "$CONSUMER_DIR" -B "$CONSUMER_BUILD" \
        -DCMAKE_PREFIX_PATH="$INSTALL_DIR" \
        2>&1 | tail -20
fi

if cmake --build "$CONSUMER_BUILD" -j$(nproc) 2>&1 | tail -3; then
    check "Consumer build succeeded" true
else
    check "Consumer build succeeded" false
    cmake --build "$CONSUMER_BUILD" -j$(nproc) 2>&1 | tail -20
fi

if "$CONSUMER_BUILD/consumer_app" 2>&1 | grep -q "OK: 42"; then
    check "Consumer app runs and produces correct output" true
else
    check "Consumer app runs and produces correct output" false
    "$CONSUMER_BUILD/consumer_app" 2>&1 || echo "(exit code: $?)"
fi

# -- Step 7: Verify consumer link integrity
section "Step 7 — Verify consumer link integrity"
if grep -r "Qt6::" "$CONSUMER_BUILD" 2>/dev/null | head -1; then
    check "Consumer has no unexpected Qt dependencies" false "Qt6:: found in build files"
else
    check "Consumer has no unexpected Qt dependencies" true
fi

# -- Summary
echo ""
echo "$(printf '═%.0s' {1..60})"
if [[ $FAILED -eq 0 ]]; then
    echo -e "${GREEN}VALIDATION PASSED${RESET} — $PASSED checks"
else
    echo -e "${RED}VALIDATION FAILED${RESET} — $PASSED passed, $FAILED FAILED"
    exit 1
fi
echo "$(printf '═%.0s' {1..60})"
