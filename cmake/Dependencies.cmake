# ==========================================================================
# Dependencies.cmake
# --------------------------------------------------------------------------
# Master orchestrator for the deterministic dependency management system.
# Included once from the root CMakeLists.txt.
#
# Architecture:
#
#   DependencyVersions.cmake   Version pins + SHA256 checksums
#           |
#   DependencyHelpers.cmake    Core: download, verify, extract, cache
#           |
#   dependencies/<Name>.cmake  Per-dependency: build options + require call
#
# No module outside Dependencies.cmake may invoke file(DOWNLOAD),
# FetchContent, or add_subdirectory for third-party dependencies.
# ==========================================================================

include_guard()

include(${CMAKE_CURRENT_LIST_DIR}/DependencyVersions.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/DependencyHelpers.cmake)

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

include(${CMAKE_CURRENT_LIST_DIR}/dependencies/OpenSSL.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/dependencies/LibUSB.cmake)
