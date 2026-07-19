# SDKDistribution.cmake — Release Infrastructure (NOT Build Infrastructure).
#
# Invoked via:  cmake -P release/SDKDistribution.cmake
#
# This script is self-contained. It never reads CMakeCache.txt,
# never references individual files or libraries, and never modifies
# the build tree. It relies exclusively on the install() tree as the
# single source of truth.
#
# Prerequisites:
#   1. Project must be configured and built (cmake --build).
#   2. cmake install must have been run at least once into the build tree.
#
# Required variables (passed via -D, BEFORE -P):
#   BUILD_DIR              — Path to the build directory
#   MBOOTCORE_SOURCE_DIR   — Path to the project source directory
#   DISTRIBUTION_MODE      — stage | archive | release (alias for archive)
#
# Usage:
#   cmake -DBUILD_DIR=build \
#         -DMBOOTCORE_SOURCE_DIR=. \
#         -DDISTRIBUTION_MODE=archive \
#         -P release/SDKDistribution.cmake

# ═══════════════════════════════════════════════════════════════════════════
# Validation
# ═══════════════════════════════════════════════════════════════════════════

if(NOT DEFINED BUILD_DIR OR BUILD_DIR STREQUAL "")
    message(FATAL_ERROR "SDKDistribution: BUILD_DIR not set.")
endif()

if(NOT DEFINED MBOOTCORE_SOURCE_DIR OR MBOOTCORE_SOURCE_DIR STREQUAL "")
    message(FATAL_ERROR "SDKDistribution: MBOOTCORE_SOURCE_DIR not set.")
endif()

if(NOT DEFINED DISTRIBUTION_MODE OR DISTRIBUTION_MODE STREQUAL "")
    message(FATAL_ERROR "SDKDistribution: DISTRIBUTION_MODE not set.\n"
        "  Valid values: stage, archive, release")
endif()

# Normalize alias and validate mode.
if(DISTRIBUTION_MODE STREQUAL "release")
    set(DISTRIBUTION_MODE "archive")
endif()
if(NOT DISTRIBUTION_MODE MATCHES "^(stage|archive)$")
    message(FATAL_ERROR
        "SDKDistribution: invalid DISTRIBUTION_MODE='${DISTRIBUTION_MODE}'\n"
        "  Valid values: stage, archive, release")
endif()

# ═══════════════════════════════════════════════════════════════════════════
# Read build metadata (generated at configure time)
# ═══════════════════════════════════════════════════════════════════════════

set(_build_info "${BUILD_DIR}/cmake/MBootCoreBuildInfo.cmake")
if(NOT EXISTS "${_build_info}")
    message(FATAL_ERROR
        "SDKDistribution: Build metadata not found:\n"
        "  ${_build_info}\n"
        "  Configure the project first: cmake -B build <options>")
endif()

include("${_build_info}")

# ═══════════════════════════════════════════════════════════════════════════
# Platform and archive format
# ═══════════════════════════════════════════════════════════════════════════

# Platform name comes from BuildMetadata.cmake.in (configured at build time).
# We do NOT use WIN32 here — that would be host-dependent when running
# via cmake -P, which is wrong for cross-compilation scenarios.
# MBOOTCORE_PLATFORM_NAME is the single source of truth.

if(MBOOTCORE_PLATFORM_NAME MATCHES "^windows-")
    set(_archive_ext "zip")
else()
    set(_archive_ext "tar.gz")
endif()

# ═══════════════════════════════════════════════════════════════════════════
# Derive archive name, install prefix, and output directory
# ═══════════════════════════════════════════════════════════════════════════

set(_versioned_dir "MBootCore-${MBOOTCORE_VERSION}")
set(_sdk_archive   "MBootCore-${MBOOTCORE_VERSION}-${MBOOTCORE_PLATFORM_NAME}-${MBOOTCORE_ARCH}")

# The install prefix IS the archive root. The versioned directory name
# becomes the root entry inside the archive:
#   MBootCore-1.0.0/
#     ├── include/
#     ├── lib/
#     ├── docs/
#     └── ...
set(_install_prefix "${BUILD_DIR}/_sdk_install/${_versioned_dir}")

# Archive output — separate from build tree.
set(_dist_dir "${MBOOTCORE_SOURCE_DIR}/dist")
set(_archive_path "${_dist_dir}/${_sdk_archive}.${_archive_ext}")

# ═══════════════════════════════════════════════════════════════════════════
# Stage: install + metadata
# ═══════════════════════════════════════════════════════════════════════════

message(STATUS "")
message(STATUS "=== MBootCore SDK Distribution ===")
message(STATUS "  Mode:     ${DISTRIBUTION_MODE}")
message(STATUS "  Version:  ${MBOOTCORE_VERSION}")
message(STATUS "  Platform: ${MBOOTCORE_PLATFORM_NAME}")
message(STATUS "  Arch:     ${MBOOTCORE_ARCH}")
message(STATUS "  Archive:  ${_sdk_archive}.${_archive_ext}")
message(STATUS "")

# Clean previous staging if it exists.
set(_sdk_base "${BUILD_DIR}/_sdk_install")
if(EXISTS "${_sdk_base}")
    file(REMOVE_RECURSE "${_sdk_base}")
endif()

# Install to a clean staging directory.
execute_process(
    COMMAND "${CMAKE_COMMAND}" --install "${BUILD_DIR}"
            --prefix "${_install_prefix}"
    RESULT_VARIABLE _install_result
)
if(NOT _install_result EQUAL 0)
    message(FATAL_ERROR
        "SDKDistribution: cmake --install failed (${_install_result}).\n"
        "  Ensure the project is built and install rules are defined.")
endif()

# Validate the install produced a usable tree.
if(NOT EXISTS "${_install_prefix}" OR
   NOT IS_DIRECTORY "${_install_prefix}/include")
    message(FATAL_ERROR
        "SDKDistribution: install tree is incomplete.\n"
        "  Expected: ${_install_prefix}/include/\n"
        "  Check cmake/InstallRules.cmake for missing install() calls.")
endif()

# Generate metadata (SDK_INFO.json, manifest.json, BUILD_INFO.txt).
# All -D parameters MUST appear before -P.
# Each -D is a single quoted argument — the value must NOT have extra quotes.
execute_process(
    COMMAND "${CMAKE_COMMAND}"
            "-DSTAGING_DIR=${_install_prefix}"
            "-DSDK_PLATFORM=${MBOOTCORE_PLATFORM_NAME}"
            "-DSDK_ARCH=${MBOOTCORE_ARCH}"
            "-DMBOOTCORE_VERSION=${MBOOTCORE_VERSION}"
            "-DSDK_ARCHIVE_NAME=${_sdk_archive}.${_archive_ext}"
            "-DMBOOTCORE_SOURCE_DIR=${MBOOTCORE_SOURCE_DIR}"
            "-DMBOOTCORE_DOCDIR=${MBOOTCORE_DOCDIR}"
            "-DMBOOTCORE_COMPILER=${MBOOTCORE_COMPILER}"
            "-DMBOOTCORE_COMPILER_VERSION=${MBOOTCORE_COMPILER_VERSION}"
            "-DMBOOTCORE_CXX_STANDARD=${MBOOTCORE_CXX_STANDARD}"
            "-DMBOOTCORE_GENERATOR=${MBOOTCORE_GENERATOR}"
            "-DMBOOTCORE_CMAKE_VERSION=${MBOOTCORE_CMAKE_VERSION}"
            "-DMBOOTCORE_BUILD_TYPE=${MBOOTCORE_BUILD_TYPE}"
            -P "${CMAKE_CURRENT_LIST_DIR}/SDKStaging.cmake"
    RESULT_VARIABLE _staging_result
)
if(NOT _staging_result EQUAL 0)
    message(FATAL_ERROR
        "SDKDistribution: SDKStaging failed (${_staging_result}).")
endif()

# ═══════════════════════════════════════════════════════════════════════════
# Archive: compress the install tree
# ═══════════════════════════════════════════════════════════════════════════

if(DISTRIBUTION_MODE STREQUAL "stage")
    message(STATUS "")
    message(STATUS "Staging complete. Install tree:")
    message(STATUS "  ${_install_prefix}")
    return()
endif()

# Create dist/ directory for the archive output.
file(MAKE_DIRECTORY "${_dist_dir}")

# Archive from the base directory so the versioned directory name
# becomes the root entry inside the archive.
if(_archive_ext STREQUAL "zip")
    execute_process(
        COMMAND "${CMAKE_COMMAND}" -E tar cf "${_archive_path}"
                --format=zip
                "${_versioned_dir}"
        WORKING_DIRECTORY "${_sdk_base}"
        RESULT_VARIABLE _archive_result
    )
else()
    execute_process(
        COMMAND "${CMAKE_COMMAND}" -E tar czf "${_archive_path}"
                --format=gnutar
                "${_versioned_dir}"
        WORKING_DIRECTORY "${_sdk_base}"
        RESULT_VARIABLE _archive_result
    )
endif()

if(NOT _archive_result EQUAL 0)
    message(FATAL_ERROR
        "SDKDistribution: archive creation failed (${_archive_result}).\n"
        "  Output: ${_archive_path}")
endif()

# Verify the archive was actually created.
# Some archiver tools on certain platforms may return exit code 0
# without producing the file (e.g. path or permission issues).
if(NOT EXISTS "${_archive_path}")
    message(FATAL_ERROR
        "SDKDistribution: archive was not created:\n"
        "  ${_archive_path}")
endif()

# Report the result.
file(SIZE "${_archive_path}" _archive_size)
math(EXPR _archive_mb "${_archive_size} / (1024 * 1024)")

message(STATUS "")
message(STATUS "SDK archive created successfully:")
message(STATUS "  Path: ${_archive_path}")
message(STATUS "  Size: ${_archive_mb} MB")
message(STATUS "")
message(STATUS "Archive root:")
message(STATUS "  ${_versioned_dir}/")
message(STATUS "    include/")
message(STATUS "    lib/")
message(STATUS "    docs/")
