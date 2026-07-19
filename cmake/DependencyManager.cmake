# ==========================================================================
# DependencyManager.cmake
# --------------------------------------------------------------------------
# Master orchestrator for the deterministic dependency management system.
# Included once from the root CMakeLists.txt.
#
# Layers:
#   DependencyVersions.cmake   Version pins + SHA256 checksums
#   DependencyManifest.cmake   Abstract dependency definitions
#   DependencyHelpers.cmake    Core: download, verify, extract, cache
#   dependencies/<Name>.cmake  Per-dependency: build options + require
#
# Only this file and dependencies/<Name>.cmake may call
# mbootcore_require_dependency().  No other module invokes file(DOWNLOAD),
# FetchContent, or add_subdirectory for third-party deps.
# ==========================================================================

include_guard()

include(${CMAKE_CURRENT_LIST_DIR}/DependencyVersions.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/DependencyManifest.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/DependencyHelpers.cmake)

option(MBOOTCORE_OFFLINE_BUILD
    "Fail with guidance if a dependency is not already cached in deps/"
    OFF
)

mbootcore_init_deps()

# ---------------------------------------------------------------------------
# Required dependencies (always built)
# ---------------------------------------------------------------------------
include(${CMAKE_CURRENT_LIST_DIR}/dependencies/NlohmannJson.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/dependencies/Zlib.cmake)

# ---------------------------------------------------------------------------
# Optional dependencies (feature-gated)
# ---------------------------------------------------------------------------
option(MBOOTCORE_ENABLE_USB "Enable USB transport backend" ON)

# ---------------------------------------------------------------------------
# Optional dependencies (feature-gated)
# ---------------------------------------------------------------------------
option(MBOOTCORE_ENABLE_CRYPTO
    "Enable cryptographic backend (Mbed TLS, auto-managed)."
    ON
)

include(${CMAKE_CURRENT_LIST_DIR}/dependencies/MbedTLS.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/dependencies/LibUSB.cmake)
