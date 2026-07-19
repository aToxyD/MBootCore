# SDKStaging.cmake — Metadata generator for SDK Distribution.
#
# Invoked via:  cmake -P release/SDKStaging.cmake
#
# Generates three metadata files in the staging directory:
#   - SDK_INFO.json     (root, schema version 1)
#   - manifest.json     (docs directory, machine-readable)
#   - BUILD_INFO.txt    (docs directory, human-readable prose)
#
# Required variables (passed via -D, BEFORE -P):
#   STAGING_DIR              — Root of the install tree
#   SDK_PLATFORM             — Platform name (e.g. windows-mingw)
#   SDK_ARCH                 — Architecture (e.g. x64)
#   MBOOTCORE_VERSION        — Version string
#   SDK_ARCHIVE_NAME         — Full archive filename with extension
#   MBOOTCORE_SOURCE_DIR     — Project source root (for Git metadata)
#   MBOOTCORE_DOCDIR         — Documentation install directory
#   MBOOTCORE_COMPILER       — Compiler ID (e.g. GNU, MSVC, AppleClang)
#   MBOOTCORE_COMPILER_VERSION — Compiler version
#   MBOOTCORE_CXX_STANDARD   — C++ standard (e.g. 17)
#   MBOOTCORE_GENERATOR      — CMake generator
#   MBOOTCORE_CMAKE_VERSION  — CMake version
#   MBOOTCORE_BUILD_TYPE     — Build type (e.g. Debug, Release)

# ═══════════════════════════════════════════════════════════════════════════
# Validation
# ═══════════════════════════════════════════════════════════════════════════

foreach(_var STAGING_DIR SDK_PLATFORM SDK_ARCH MBOOTCORE_VERSION
            SDK_ARCHIVE_NAME MBOOTCORE_SOURCE_DIR MBOOTCORE_DOCDIR
            MBOOTCORE_COMPILER MBOOTCORE_COMPILER_VERSION
            MBOOTCORE_CXX_STANDARD MBOOTCORE_GENERATOR
            MBOOTCORE_CMAKE_VERSION)
    if(NOT DEFINED ${_var})
        message(FATAL_ERROR "SDKStaging: ${_var} not set.")
    endif()
    if("${${_var}}" STREQUAL "")
        message(FATAL_ERROR "SDKStaging: ${_var} is empty.")
    endif()
endforeach()

# MBOOTCORE_BUILD_TYPE may be empty for single-config generators
# (e.g. MinGW Makefiles). Default to "Release" for metadata.
if(NOT DEFINED MBOOTCORE_BUILD_TYPE)
    set(MBOOTCORE_BUILD_TYPE "Release")
endif()
if("${MBOOTCORE_BUILD_TYPE}" STREQUAL "")
    set(MBOOTCORE_BUILD_TYPE "Release")
endif()

if(NOT IS_DIRECTORY "${STAGING_DIR}")
    message(FATAL_ERROR "SDKStaging: staging directory does not exist:\n"
        "  ${STAGING_DIR}")
endif()

# ═══════════════════════════════════════════════════════════════════════════
# JSON string escaping
# ═══════════════════════════════════════════════════════════════════════════

function(json out value)
    string(REPLACE "\\" "\\\\" _escaped "${value}")
    string(REPLACE "\"" "\\\"" _escaped "${_escaped}")
    string(REPLACE "\n" "\\n"  _escaped "${_escaped}")
    string(REPLACE "\r" "\\r"  _escaped "${_escaped}")
    string(REPLACE "\t" "\\t"  _escaped "${_escaped}")
    set(${out} "${_escaped}" PARENT_SCOPE)
endfunction()

# ═══════════════════════════════════════════════════════════════════════════
# Detect documentation directory
# ═══════════════════════════════════════════════════════════════════════════

set(_docs_dir "${STAGING_DIR}/${MBOOTCORE_DOCDIR}")
if(NOT IS_DIRECTORY "${_docs_dir}")
    message(FATAL_ERROR
        "SDKStaging: documentation directory not found:\n"
        "  ${_docs_dir}\n"
        "  Check cmake/InstallRules.cmake — SDK is incomplete.")
endif()

# ═══════════════════════════════════════════════════════════════════════════
# Collect Git metadata (best-effort, non-fatal)
# ═══════════════════════════════════════════════════════════════════════════

set(_git_commit "unknown")
set(_git_branch "unknown")

find_program(GIT_EXECUTABLE git)
if(GIT_EXECUTABLE)
    execute_process(
        COMMAND "${GIT_EXECUTABLE}" rev-parse --short=12 HEAD
        WORKING_DIRECTORY "${MBOOTCORE_SOURCE_DIR}"
        OUTPUT_VARIABLE _git_commit
        ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE
        TIMEOUT 5
    )
    if(NOT _git_commit)
        set(_git_commit "unknown")
    endif()

    execute_process(
        COMMAND "${GIT_EXECUTABLE}" rev-parse --abbrev-ref HEAD
        WORKING_DIRECTORY "${MBOOTCORE_SOURCE_DIR}"
        OUTPUT_VARIABLE _git_branch
        ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE
        TIMEOUT 5
    )
    if(NOT _git_branch)
        set(_git_branch "unknown")
    endif()
endif()

# ═══════════════════════════════════════════════════════════════════════════
# Build timestamp
# ═══════════════════════════════════════════════════════════════════════════

string(TIMESTAMP _build_date "%Y-%m-%dT%H:%M:%SZ" UTC)

# ═══════════════════════════════════════════════════════════════════════════
# Escape all interpolated strings for JSON safety
# ═══════════════════════════════════════════════════════════════════════════

json(_esc_version     "${MBOOTCORE_VERSION}")
json(_esc_platform    "${SDK_PLATFORM}")
json(_esc_arch        "${SDK_ARCH}")
json(_esc_archive     "${SDK_ARCHIVE_NAME}")
json(_esc_compiler    "${MBOOTCORE_COMPILER}")
json(_esc_comp_ver    "${MBOOTCORE_COMPILER_VERSION}")
json(_esc_cxx_std     "${MBOOTCORE_CXX_STANDARD}")
json(_esc_generator   "${MBOOTCORE_GENERATOR}")
json(_esc_cmake_ver   "${MBOOTCORE_CMAKE_VERSION}")
json(_esc_build_type  "${MBOOTCORE_BUILD_TYPE}")
json(_esc_git_commit  "${_git_commit}")
json(_esc_git_branch  "${_git_branch}")

# ═══════════════════════════════════════════════════════════════════════════
# SDK_INFO.json — root of the install tree
# ═══════════════════════════════════════════════════════════════════════════

file(WRITE "${STAGING_DIR}/SDK_INFO.json" "{
    \"schema\": 1,
    \"name\": \"MBootCore\",
    \"version\": \"${_esc_version}\",
    \"platform\": \"${_esc_platform}\",
    \"architecture\": \"${_esc_arch}\",
    \"type\": \"Developer SDK\"
}
")

# ═══════════════════════════════════════════════════════════════════════════
# manifest.json — documentation directory
# ═══════════════════════════════════════════════════════════════════════════

file(WRITE "${_docs_dir}/manifest.json" "{
    \"schema\": 1,
    \"version\": \"${_esc_version}\",
    \"platform\": \"${_esc_platform}\",
    \"architecture\": \"${_esc_arch}\",
    \"archive\": \"${_esc_archive}\",
    \"compiler\": \"${_esc_compiler}\",
    \"compiler_version\": \"${_esc_comp_ver}\",
    \"cxx_standard\": \"${_esc_cxx_std}\",
    \"generator\": \"${_esc_generator}\",
    \"cmake_version\": \"${_esc_cmake_ver}\",
    \"build_type\": \"${_esc_build_type}\",
    \"git_commit\": \"${_esc_git_commit}\",
    \"git_branch\": \"${_esc_git_branch}\"
}
")

# ═══════════════════════════════════════════════════════════════════════════
# BUILD_INFO.txt — documentation directory (human-readable, not JSON)
# ═══════════════════════════════════════════════════════════════════════════

file(WRITE "${_docs_dir}/BUILD_INFO.txt" "MBootCore SDK Build Information
================================

Version:      ${MBOOTCORE_VERSION}
Platform:     ${SDK_PLATFORM}
Architecture: ${SDK_ARCH}
Compiler:     ${MBOOTCORE_COMPILER} ${MBOOTCORE_COMPILER_VERSION}
C++ Standard: ${MBOOTCORE_CXX_STANDARD}
Generator:    ${MBOOTCORE_GENERATOR}
CMake:        ${MBOOTCORE_CMAKE_VERSION}
Build Type:   ${MBOOTCORE_BUILD_TYPE}
Git Commit:   ${_git_commit}
Git Branch:   ${_git_branch}
Archive:      ${SDK_ARCHIVE_NAME}
Generated:    ${_build_date}

This SDK was built from the MBootCore project and includes:
  - Compiled libraries (lib/)
  - Public headers (include/)
  - Documentation (docs/)
  - License files (LICENSE, NOTICE, ThirdPartyLicenses.md)

For build instructions, see the documentation.
")

# ═══════════════════════════════════════════════════════════════════════════
# Done
# ═══════════════════════════════════════════════════════════════════════════

message(STATUS "SDKStaging: metadata written to ${STAGING_DIR}")
