#!/usr/bin/env pwsh
# =============================================================================
# MBootCore — Release Pipeline (thin wrapper)
# create-release.ps1
#
# Orchestrates: configure → build → test → export validation → SDK archive
# All packaging logic lives in release/SDKDistribution.cmake.
#
# Usage:
#   .\scripts\create-release.ps1
#   .\scripts\create-release.ps1 -Mode minimal
#   .\scripts\create-release.ps1 -SkipTests -SkipExportValidation
#
# Modes:
#   full     — Release build with all components (default)
#   minimal  — Release build, core library only
#   nightly  — Debug build with ASan + UBSan
#   coverage — Debug build with code coverage
#
# Requirements:
#   - winlibs MinGW-w64 at C:\winlibs\mingw32
#   - Qt 6.11.1 static build at C:\Qt\6.11.1-static\mingw_32
#   - CMake 3.20+ on PATH
# =============================================================================

param(
    [string]$Mode                  = "full",
    [string]$QtPrefix            = "C:/Qt/6.11.1-static/mingw_32",
    [string]$MinGWBin            = "C:/winlibs/mingw32/bin",
    [switch]$SkipTests,
    [switch]$SkipExportValidation,
    [switch]$Verbose
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

# ── Mode → Preset mapping ───────────────────────────────────────────────────
# Preset names are implementation details; users see human-readable modes.
switch ($Mode) {
    "full"     { $Preset = "release-package";  $BuildDir = "build/release-package" }
    "minimal"  { $Preset = "release";       $BuildDir = "build/release" }
    "nightly"  { $Preset = "asan-ubsan";    $BuildDir = "build/asan-ubsan" }
    "coverage" { $Preset = "coverage";      $BuildDir = "build/coverage" }
    default    { Fail "Unknown mode: $Mode (valid: full, minimal, nightly, coverage)" }
}

# ── Helpers ──────────────────────────────────────────────────────────────────
$GREEN  = "`e[32m"
$RED    = "`e[31m"
$YELLOW = "`e[33m"
$RESET  = "`e[0m"
$STEP   = 0

function Step([string]$name) {
    $script:STEP++
    Write-Host ""
    Write-Host "${GREEN}[$script:STEP]${RESET} $name"
    Write-Host ("-" * 60)
}

function Ok([string]$msg)   { Write-Host "  ${GREEN}->$RESET $msg" }
function Warn([string]$msg) { Write-Host "  ${YELLOW}!!$RESET $msg" }
function Fail([string]$msg) { Write-Host "  ${RED}XX$RESET $msg"; throw "Release failed: $msg" }

function RunCmd([string]$cmd, [string[]]$arguments) {
    if ($Verbose) { Write-Host "  > $cmd $($arguments -join ' ')" }
    & $cmd @arguments
    if ($LASTEXITCODE -ne 0) { Fail "'$cmd' exited with code $($LASTEXITCODE)" }
}

# ── Test Classification ──────────────────────────────────────────────────
$EnvironmentTests = [System.Collections.Generic.HashSet[string]]::new(
    [System.StringComparer]::OrdinalIgnoreCase
)

@(
    "backend_selection_test",
    "transport_test"
) | ForEach-Object {
    [void]$EnvironmentTests.Add($_)
}

$testResult          = "passed"
$environmentFailures = [System.Collections.Generic.List[string]]::new()
$productFailures     = [System.Collections.Generic.List[string]]::new()

# ── Entry Point ──────────────────────────────────────────────────────────────
$TotalSw = [System.Diagnostics.Stopwatch]::StartNew()
$ProjectRoot = Split-Path $PSScriptRoot -Parent

try {
    # ── 1. Environment Setup ─────────────────────────────────────────────────
    Step "Environment Setup"

    $env:Path = "$MinGWBin;$env:Path"
    $Version = (Get-Content "$ProjectRoot/VERSION").Trim()

    Ok "Version: $Version"
    Ok "Mode: $Mode"

    foreach ($tool in @("cmake", "ctest", "g++", "mingw32-make")) {
        if (-not (Get-Command $tool -ErrorAction SilentlyContinue)) {
            Fail "$tool not found on PATH"
        }
        Ok "$tool found"
    }

    # ── 2. Configure ─────────────────────────────────────────────────────────
    Step "CMake Configure"

    $BuildPath = Join-Path $ProjectRoot $BuildDir
    if (-not (Test-Path (Join-Path $ProjectRoot "CMakeLists.txt"))) {
        Fail "Project root does not contain CMakeLists.txt: $ProjectRoot"
    }
    if (-not (Test-Path $BuildPath)) { New-Item -ItemType Directory $BuildPath | Out-Null }

    Ok "Source: $ProjectRoot"
    Ok "Build : $BuildPath"

    Push-Location $BuildPath
    try {
        $cmakeArgs = @(
            "-S", $ProjectRoot,
            "-B", $BuildPath,
            "-G", "MinGW Makefiles",
            "-DCMAKE_BUILD_TYPE=Release",
            "-DMBOOTCORE_BUILD_TESTS=ON",
            "-DMBOOTCORE_BUILD_SECURITY_TESTS=ON",
            "-DMBOOTCORE_BUILD_TOOLS=ON",
            "-DMBOOTCORE_BUILD_CLI=ON",
            "-DMBOOTCORE_BUILD_STUDIO=ON",
            "-DMBOOTCORE_BUILD_EXAMPLES=ON",
            "-DCMAKE_PREFIX_PATH=$QtPrefix"
        )
        if ($Verbose) { $cmakeArgs += "-DCMAKE_VERBOSE_MAKEFILE=ON" }

        RunCmd "cmake" $cmakeArgs
        Ok "Configure complete"
    } finally { Pop-Location }

    # ── 3. Build ─────────────────────────────────────────────────────────────
    Step "CMake Build"

    Push-Location $BuildPath
    try {
        RunCmd "cmake" @("--build", ".", "-j4")
        Ok "Build complete"
    } finally { Pop-Location }

    # ── 4. Tests ─────────────────────────────────────────────────────────────
    if (-not $SkipTests) {
        Step "Test Suite"

        Push-Location $BuildPath
        try {
            $failedLogPath = Join-Path $BuildPath "Testing/Temporary/LastTestsFailed.log"
            $junitPath     = Join-Path $BuildPath "Testing/Temporary/ctest-results.xml"

            if (Test-Path -Path $failedLogPath -PathType Leaf) {
                Remove-Item -LiteralPath $failedLogPath -Force
            }
            if (Test-Path -Path $junitPath -PathType Leaf) {
                Remove-Item -LiteralPath $junitPath -Force
            }

            & ctest --output-on-failure -j4 --output-junit "$junitPath"
            $ctestExitCode = $LASTEXITCODE

            $failedTests = [System.Collections.Generic.List[string]]::new()

            if ($ctestExitCode -ne 0) {
                if (Test-Path -Path $junitPath -PathType Leaf) {
                    try {
                        [xml]$xml = Get-Content -LiteralPath $junitPath -Raw
                        foreach ($node in $xml.SelectNodes("//testcase[failure]")) {
                            $failedTests.Add($node.name)
                        }
                    } catch {
                        Fail "Failed to parse JUnit XML: $_"
                    }
                } elseif (Test-Path -Path $failedLogPath -PathType Leaf) {
                    $logContent = Get-Content -LiteralPath $failedLogPath -Raw
                    if ([string]::IsNullOrWhiteSpace($logContent)) {
                        Fail "ctest exited with code $ctestExitCode but LastTestsFailed.log is empty"
                    }
                    foreach ($line in Get-Content -LiteralPath $failedLogPath) {
                        $parts = $line.Split(":", 2)
                        if ($parts.Count -eq 2) {
                            $failedTests.Add($parts[1].Trim())
                        }
                    }
                } else {
                    Fail "ctest exited with code $ctestExitCode but no test results found"
                }

                if ($failedTests.Count -eq 0) {
                    Fail "ctest exited with code $ctestExitCode but no failed test names could be identified"
                }
            }

            foreach ($test in $failedTests) {
                if ($EnvironmentTests.Contains($test)) {
                    $environmentFailures.Add($test)
                } else {
                    $productFailures.Add($test)
                }
            }

            if ($productFailures.Count -gt 0) {
                $testResult = "product_failed"
                Write-Host ""
                Write-Host "${RED}XX${RESET} Unexpected product test failures:"
                foreach ($t in $productFailures) {
                    Write-Host "    $t"
                }
                Fail "Product tests failed"
            } elseif ($environmentFailures.Count -gt 0) {
                $testResult = "environment_failed"
                Write-Host ""
                Write-Host "${YELLOW}!!${RESET} Environment tests failed (non-blocking):"
                foreach ($t in $environmentFailures) {
                    Write-Host "    $t"
                }
            } else {
                Ok "All tests passed"
            }
        } finally { Pop-Location }
    } else {
        $testResult = "skipped"
        Warn "Tests skipped"
    }

    # ── 5. Export Validation ─────────────────────────────────────────────────
    if (-not $SkipExportValidation) {
        Step "Export Validation"

        Push-Location $BuildPath
        try {
            & ctest --output-on-failure -R "export_validation"
            if ($LASTEXITCODE -ne 0) { Fail "Export validation failed" }
            Ok "Export validation passed"
        } finally { Pop-Location }
    } else {
        Warn "Export validation skipped"
    }

    # ── 6. SDK Archive ───────────────────────────────────────────────────────
    Step "SDK Archive"

    $sdkScript = Join-Path $ProjectRoot "release/SDKDistribution.cmake"
    if (-not (Test-Path $sdkScript)) { Fail "SDKDistribution.cmake not found: $sdkScript" }

    RunCmd "cmake" @(
        "-DBUILD_DIR=$BuildPath",
        "-DMBOOTCORE_SOURCE_DIR=$ProjectRoot",
        "-DDISTRIBUTION_MODE=archive",
        "-P", $sdkScript
    )

    # Locate the generated archive
    $distDir = Join-Path $ProjectRoot "dist"
    $archive = Get-ChildItem $distDir -Filter "*.zip" -ErrorAction SilentlyContinue |
        Sort-Object LastWriteTime -Descending | Select-Object -First 1

    # ── Summary ──────────────────────────────────────────────────────────────
    Step "Release Summary"

    $elapsed = $TotalSw.Elapsed

    Write-Host ""
    Write-Host "========================================="
    Write-Host "Release Summary"
    Write-Host "========================================="
    Write-Host ""

    Write-Host "Product Tests:"
    if ($testResult -eq "product_failed") {
        Write-Host "  FAILED"
        Write-Host ""
        Write-Host "Unexpected failing tests:"
        Write-Host ""
        foreach ($t in $productFailures) {
            Write-Host "    $t"
        }
    } elseif ($testResult -eq "skipped") {
        Write-Host "  Skipped"
    } else {
        Write-Host "  Passed"
    }
    Write-Host ""

    Write-Host "Environment Tests:"
    if ($environmentFailures.Count -gt 0) {
        Write-Host "  Failed ($($environmentFailures.Count))"
        Write-Host ""
        foreach ($t in $environmentFailures) {
            Write-Host "    $t"
        }
    } elseif ($testResult -eq "skipped") {
        Write-Host "  Skipped"
    } else {
        Write-Host "  Passed"
    }
    Write-Host ""

    Write-Host "Archive:"
    if ($archive) {
        Write-Host "  dist/$($archive.Name)"
    } else {
        Write-Host "  (not created)"
    }
    Write-Host ""

    Write-Host "Release Status:"
    if ($testResult -eq "product_failed") {
        Write-Host "  ${RED}FAILED${RESET}"
    } elseif ($environmentFailures.Count -gt 0) {
        Write-Host "  ${YELLOW}SUCCESS (environment warnings)${RESET}"
    } else {
        Write-Host "  ${GREEN}SUCCESS${RESET}"
    }
    Write-Host ""
    Write-Host "  Time: $([math]::Floor($elapsed.TotalMinutes))m $($elapsed.Seconds)s"
    Write-Host ""

} catch {
    Write-Host ""
    Write-Host "${RED}RELEASE PIPELINE FAILED${RESET}"
    Write-Host "${RED}$_${RESET}"
    exit 1
}
