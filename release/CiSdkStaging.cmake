# CiSdkStaging.cmake — Thin CI wrapper for SDKStaging.cmake.
#
# Invoked via:  cmake -P release/CiSdkStaging.cmake
#
# Reads build metadata from the configured build tree, then delegates to
# SDKStaging.cmake with the complete set of required variables.
# Contains zero CI-specific logic — platform and architecture must be
# passed explicitly.
#
# Required -D variables (passed BEFORE -P):
#   BUILD_DIR                 — Build directory path
#   STAGING_DIR               — Install tree root (e.g. _install)
#   SDK_PLATFORM              — Platform name (e.g. linux-gcc, macos-clang)
#   SDK_ARCH                  — Architecture (e.g. x64, arm64)
#   MBOOTCORE_VERSION         — Version string
#   MBOOTCORE_SOURCE_DIR      — Project source root
#
# Behaviour:
#   1. Reads MBootCoreBuildInfo.cmake from BUILD_DIR/cmake/
#   2. Derives SDK_ARCHIVE_NAME from build metadata
#   3. Delegates to SDKStaging.cmake (all variables visible via include scope)

foreach(_var BUILD_DIR STAGING_DIR SDK_PLATFORM SDK_ARCH
            MBOOTCORE_VERSION MBOOTCORE_SOURCE_DIR)
    if(NOT DEFINED ${_var})
        message(FATAL_ERROR "CiSdkStaging: ${_var} not set.")
    endif()
    if("${${_var}}" STREQUAL "")
        message(FATAL_ERROR "CiSdkStaging: ${_var} is empty.")
    endif()
endforeach()

set(_build_info "${BUILD_DIR}/cmake/MBootCoreBuildInfo.cmake")
if(NOT EXISTS "${_build_info}")
    message(FATAL_ERROR
        "CiSdkStaging: build metadata not found:\n"
        "  ${_build_info}\n"
        "  Configure the project first (cmake --preset <preset>).")
endif()
include("${_build_info}")

if(NOT DEFINED MBOOTCORE_BUILD_TYPE OR "${MBOOTCORE_BUILD_TYPE}" STREQUAL "")
    set(MBOOTCORE_BUILD_TYPE "Release")
endif()

if(MBOOTCORE_PLATFORM_NAME MATCHES "^windows-")
    set(_archive_ext "zip")
else()
    set(_archive_ext "tar.gz")
endif()
set(SDK_ARCHIVE_NAME
    "MBootCore-${MBOOTCORE_VERSION}-${MBOOTCORE_PLATFORM_NAME}-${MBOOTCORE_ARCH}.${_archive_ext}")

include(${CMAKE_CURRENT_LIST_DIR}/SDKStaging.cmake)
