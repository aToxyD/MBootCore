#!/usr/bin/env bash
# =============================================================================
# MBootCore — Release Pipeline (thin wrapper)
# create-release.sh
#
# Orchestrates: configure → build → test → export validation → SDK archive
# All packaging logic lives in release/SDKDistribution.cmake.
#
# Usage:
#   ./scripts/create-release.sh
#   ./scripts/create-release.sh --mode minimal
#   ./scripts/create-release.sh --skip-tests --skip-export-validation
#
# Modes:
#   full     — Release build with all components (default)
#   minimal  — Release build, core library only
#   nightly  — Debug build with ASan + UBSan
#   coverage — Debug build with code coverage
#
# Requirements:
#   - GCC 11+ or Clang 12+
#   - CMake 3.20+
# =============================================================================

set -euo pipefail

# ── Defaults ─────────────────────────────────────────────────────────────────
MODE="full"
SKIP_TESTS=false
SKIP_EXPORT_VALIDATION=false
ALLOW_DIRTY=false
VERBOSE=false
JOBS=$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)

# ── Colors ───────────────────────────────────────────────────────────────────
GREEN="\033[32m"
RED="\033[31m"
YELLOW="\033[33m"
RESET="\033[0m"
STEP=0

step()  { STEP=$((STEP+1)); echo ""; echo -e "${GREEN}[$STEP]${RESET} $1"; printf '%.0s-' {1..60}; echo ""; }
ok()    { echo -e "  ${GREEN}->$RESET $1"; }
warn()  { echo -e "  ${YELLOW}!!$RESET $1"; }
fail()  { echo -e "  ${RED}XX$RESET $1"; exit 1; }
runcmd(){ $VERBOSE && echo "  > $*"; "$@" || fail "Command failed: $*"; }

# ── Test Classification ──────────────────────────────────────────────────
is_environment_test() {
    local name
    name="$(printf '%s' "$1" | tr '[:upper:]' '[:lower:]')"
    case "$name" in
        backend_selection_test|transport_test) return 0 ;;
        *) return 1 ;;
    esac
}

# ── Host platform detection for release packaging ──────────────────────────
# Resolve the platform-specific release packaging preset.
# Sets PRESET and BUILD_DIR for the current host.
select_release_configuration() {
    local os arch

    os="$(uname -s)"
    arch="$(uname -m)"

    case "$os" in
        Linux)
            PRESET="release-package-verify-linux-gcc"
            BUILD_DIR="build/release-package-verify-linux-gcc"
            ;;
        Darwin)
            case "$arch" in
                arm64)
                    PRESET="release-package-verify-macos-arm64"
                    BUILD_DIR="build/release-package-verify-macos-arm64"
                    ;;
                *)
                    PRESET="release-package-verify-macos-x64"
                    BUILD_DIR="build/release-package-verify-macos-x64"
                    ;;
            esac
            ;;
        *)
            fail "Unsupported OS for release packaging: $os"
            ;;
    esac
}

test_result="passed"
environment_failures=()
product_failures=()

# ── Argument parsing ─────────────────────────────────────────────────────────
while [[ $# -gt 0 ]]; do
    case "$1" in
        --mode)                     MODE="$2";                    shift 2 ;;
        --skip-tests)              SKIP_TESTS=true;              shift ;;
        --skip-export-validation)  SKIP_EXPORT_VALIDATION=true;  shift ;;
        --allow-dirty)             ALLOW_DIRTY=true;             shift ;;
        --verbose)                 VERBOSE=true;                 shift ;;
        -j|--jobs)                 JOBS="$2";                    shift 2 ;;
        *) echo "Unknown argument: $1"; exit 1 ;;
    esac
done

# ── Mode → Preset mapping ───────────────────────────────────────────────────
# Preset names are implementation details; users see human-readable modes.
case "$MODE" in
    full)     select_release_configuration ;;
    minimal)  PRESET="release";       BUILD_DIR="build/release" ;;
    nightly)  PRESET="asan-ubsan";    BUILD_DIR="build/asan-ubsan" ;;
    coverage) PRESET="coverage";      BUILD_DIR="build/coverage" ;;
    *) echo "Unknown mode: $MODE (valid: full, minimal, nightly, coverage)"; exit 1 ;;
esac

# ── Setup ────────────────────────────────────────────────────────────────────
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
VERSION="$(tr -d '[:space:]' < "$PROJECT_ROOT/VERSION")"

cd "$PROJECT_ROOT"

ok "Version: $VERSION"
ok "Mode: $MODE"

# ── Pre-flight: source tree cleanliness ────────────────────────────────────
if [[ "$ALLOW_DIRTY" != true ]]; then
    if [[ -n "$(git status --porcelain)" ]]; then
        fail "Source tree has uncommitted or untracked changes. Use --allow-dirty to override."
    fi
    ok "Source tree is clean"
else
    ok "Source tree cleanliness overridden (--allow-dirty)"
fi

for tool in cmake ctest; do
    command -v "$tool" >/dev/null 2>&1 || fail "$tool not found on PATH"
    ok "$tool found"
done

HAVE_OUTPUT_JUNIT=false
if ctest --help 2>&1 | grep -q -- '--output-junit'; then
    HAVE_OUTPUT_JUNIT=true
fi

PIPELINE_START=$(date +%s)

# ── 1. Configure ────────────────────────────────────────────────────────────
step "CMake Configure"

if [[ "$(uname -s)" == "Darwin" ]]; then
    export MACOSX_DEPLOYMENT_TARGET=13.0
fi

runcmd cmake --preset "$PRESET" -G Ninja
ok "Configure complete"

# ── 2. Build ────────────────────────────────────────────────────────────────
step "CMake Build"

runcmd cmake --build "$BUILD_DIR" -j"$JOBS"
ok "Build complete"

# ── 3. Tests ────────────────────────────────────────────────────────────────
if [[ "$SKIP_TESTS" == "false" ]]; then
    step "Test Suite"

    FAILED_LOG="Testing/Temporary/LastTestsFailed.log"

    rm -f "$FAILED_LOG"

    if [[ "$HAVE_OUTPUT_JUNIT" == "true" ]]; then
        JUNIT_PATH="Testing/Temporary/ctest-results.xml"
        rm -f "$JUNIT_PATH"
        ctest_exit=0
        ctest --output-on-failure -j"$JOBS" --output-junit "$JUNIT_PATH" || ctest_exit=$?
    else
        ctest_exit=0
        ctest --output-on-failure -j"$JOBS" || ctest_exit=$?
    fi

    if [[ $ctest_exit -ne 0 ]]; then
        if [[ -f "$FAILED_LOG" ]]; then
            while IFS= read -r line; do
                [[ -z "$line" ]] && continue
                test_name="${line#*:}"
                if is_environment_test "$test_name"; then
                    environment_failures+=("$test_name")
                else
                    product_failures+=("$test_name")
                fi
            done < "$FAILED_LOG"
        else
            fail "ctest exited with code $ctest_exit but no test results found"
        fi
    fi

    if [[ ${#product_failures[@]} -gt 0 ]]; then
        test_result="product_failed"
        echo ""
        echo -e "  ${RED}XX${RESET} Unexpected product test failures:"
        for t in "${product_failures[@]}"; do
            echo "    $t"
        done
        fail "Product tests failed"
    elif [[ ${#environment_failures[@]} -gt 0 ]]; then
        test_result="environment_failed"
        echo ""
        echo -e "  ${YELLOW}!!${RESET} Environment tests failed (non-blocking):"
        for t in "${environment_failures[@]}"; do
            echo "    $t"
        done
    else
        ok "All tests passed"
    fi
else
    test_result="skipped"
    warn "Tests skipped"
fi

# ── 4. Export Validation ────────────────────────────────────────────────────
if [[ "$SKIP_EXPORT_VALIDATION" == "false" ]]; then
    step "Export Validation"

    runcmd ctest --output-on-failure -R "export_validation"
    ok "Export validation passed"
else
    warn "Export validation skipped"
fi

# ── 5. SDK Archive ──────────────────────────────────────────────────────────
step "SDK Archive"

SDK_SCRIPT="$PROJECT_ROOT/release/SDKDistribution.cmake"
[[ -f "$SDK_SCRIPT" ]] || fail "SDKDistribution.cmake not found: $SDK_SCRIPT"

runcmd cmake \
    -DBUILD_DIR="$PROJECT_ROOT/$BUILD_DIR" \
    -DMBOOTCORE_SOURCE_DIR="$PROJECT_ROOT" \
    -DDISTRIBUTION_MODE=archive \
    -P "$SDK_SCRIPT"

# Locate the generated archive
ARCHIVE=$(ls -t "$PROJECT_ROOT/dist/"*.zip "$PROJECT_ROOT/dist/"*.tar.gz 2>/dev/null | head -1)

# ── Summary ──────────────────────────────────────────────────────────────────
step "Release Summary"

PIPELINE_END=$(date +%s)
ELAPSED=$(( PIPELINE_END - PIPELINE_START ))
ELAPSED_MIN=$(( ELAPSED / 60 ))
ELAPSED_SEC=$(( ELAPSED % 60 ))

echo ""
echo "========================================="
echo "Release Summary"
echo "========================================="
echo ""

echo "Product Tests:"
case "$test_result" in
    product_failed)
        echo "  FAILED"
        ;;
    skipped)
        echo "  Skipped"
        ;;
    *)
        echo "  Passed"
        ;;
esac
echo ""

echo "Environment Tests:"
if [[ ${#environment_failures[@]} -gt 0 ]]; then
    echo "  Failed (${#environment_failures[@]})"
elif [[ "$test_result" == "skipped" ]]; then
    echo "  Skipped"
else
    echo "  Passed"
fi
echo ""

echo "Archive:"
if [[ -n "${ARCHIVE:-}" ]]; then
    echo "  dist/$(basename "$ARCHIVE")"
else
    echo "  (not created)"
fi
echo ""

echo "Release Status:"
if [[ "$test_result" == "product_failed" ]]; then
    echo -e "  ${RED}FAILED${RESET}"
elif [[ ${#environment_failures[@]} -gt 0 ]]; then
    echo -e "  ${YELLOW}SUCCESS (environment warnings)${RESET}"
else
    echo -e "  ${GREEN}SUCCESS${RESET}"
fi
echo ""
echo "  Time: ${ELAPSED_MIN}m ${ELAPSED_SEC}s"
echo ""
