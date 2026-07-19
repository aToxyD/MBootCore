# ==========================================================================
# dependencies/NlohmannJson.cmake
# --------------------------------------------------------------------------
# Header-only JSON library.  Always required.
# ==========================================================================

 message(STATUS "")

 mbootcore_require_dependency(NAME nlohmann_json)

 # Header-only: no objects to merge, no install, no export.
 # Used internally during build only.

 message(STATUS "[deps] nlohmann_json: header-only, internal use only")
