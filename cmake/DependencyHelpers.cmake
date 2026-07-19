# ==========================================================================
# DependencyHelpers.cmake
# --------------------------------------------------------------------------
# Core helper functions for the deterministic dependency manager.
# File download, SHA256 verification, archive extraction, caching, and
# build wiring.
#
# Caching model:
#   deps/<name>/
#       cache/     → <name>.tar.gz (verified on-disk archive)
#       source/    → extracted source tree (CMakeLists.txt at root)
#       .version   → plain text version stamp
# ==========================================================================

include_guard(GLOBAL)

# ---------------------------------------------------------------------------
# mbootcore_init_deps
# ---------------------------------------------------------------------------
function(mbootcore_init_deps)
    set(MBOOTCORE_DEPS_DIR "${CMAKE_SOURCE_DIR}/deps" PARENT_SCOPE)
    # No directories are pre-created — each dependency creates its own.
endfunction()

# ---------------------------------------------------------------------------
# mbootcore_require_dependency
#
# Usage:
#   mbootcore_require_dependency(NAME <name>)
#
# Reads NAME, VERSION, URL, SHA256 from the manifest variables set by
# DependencyManifest.cmake.
#
# Behaviour:
#   OFFLINE_BUILD=ON and source exists → reuse (no network)
#   OFFLINE_BUILD=ON and source missing → FATAL_ERROR
#   cache exists + version matches    → reuse
#   cache missing or version changed  → download + verify + extract + build
# ---------------------------------------------------------------------------
function(mbootcore_require_dependency)
    cmake_parse_arguments(DEP "SOURCE_ONLY" "NAME" "" ${ARGN})
    if(NOT DEP_NAME)
        message(FATAL_ERROR "mbootcore_require_dependency: NAME is required")
    endif()

    # Read manifest fields from MBOOTCORE_DEP_* variables.
    set(_name    "${DEP_NAME}")
    set(_version "${MBOOTCORE_DEP_${_name}_VERSION}")
    set(_url     "${MBOOTCORE_DEP_${_name}_URL}")
    set(_sha256  "${MBOOTCORE_DEP_${_name}_SHA256}")

    if(NOT _version OR NOT _url OR NOT _sha256)
        message(FATAL_ERROR
            "[deps] ${_name}: dependency not found in manifest. "
            "Add it to cmake/DependencyManifest.cmake"
        )
    endif()

    # Paths — everything lives under a single directory per dependency.
    set(_dep_dir    "${CMAKE_SOURCE_DIR}/deps/${_name}")
    set(_cache_file "${_dep_dir}/cache/${_name}.tar.gz")
    set(_source     "${_dep_dir}/source")
    set(_stamp      "${_dep_dir}/.version")
    set(_deps_build "${CMAKE_BINARY_DIR}/_deps_build/${_name}")

    # ------------------------------------------------------------------
    # Helper: read version stamp.
    # ------------------------------------------------------------------
    if(EXISTS "${_stamp}")
        file(READ "${_stamp}" _cached_version)
        string(STRIP "${_cached_version}" _cached_version)
    else()
        set(_cached_version "")
    endif()

    # ------------------------------------------------------------------
    # Helper: verify SHA256 of an existing archive.
    # ------------------------------------------------------------------
    if(EXISTS "${_cache_file}")
        file(SHA256 "${_cache_file}" _existing_hash)
    else()
        set(_existing_hash "")
    endif()

    # ------------------------------------------------------------------
    # Helper: check whether source directory is valid.
    # Source is valid if it exists, is non-empty, and the version stamp
    # matches the requested version.
    # ------------------------------------------------------------------
    if(EXISTS "${_source}" AND _cached_version)
        file(GLOB _source_content "${_source}/*")
        if(_source_content)
            set(_source_valid TRUE)
        else()
            set(_source_valid FALSE)
        endif()
    else()
        set(_source_valid FALSE)
    endif()

    # ------------------------------------------------------------------
    # Check OFFLINE_BUILD mode.
    # ------------------------------------------------------------------
    if(MBOOTCORE_OFFLINE_BUILD)
        if(_source_valid AND _cached_version STREQUAL _version)
            message(STATUS "[deps] ${_name}: cached at ${_version} (offline)")
            if(NOT DEP_SOURCE_ONLY)
                add_subdirectory("${_source}" "${_deps_build}")
            else()
                set(MBOOTCORE_DEP_${_name}_SOURCE_DIR "${_source}" PARENT_SCOPE)
                set(MBOOTCORE_DEP_${_name}_BUILD_DIR "${_deps_build}" PARENT_SCOPE)
            endif()
            return()
        else()
            message(FATAL_ERROR
                "[deps] OFFLINE_BUILD=ON but ${_name} is not cached at ${_version}.\n"
                "  Path checked: ${_source}\n"
                "  Either connect to the Internet and disable OFFLINE_BUILD, or\n"
                "  populate the cache manually (download ${_url} to ${_cache_file})."
            )
        endif()
    endif()

    # ------------------------------------------------------------------
    # Case 1: source valid + version matches — reuse immediately.
    # ------------------------------------------------------------------
    if(_source_valid AND _cached_version STREQUAL _version)
        message(STATUS "[deps] ${_name}: cached at ${_version}, reusing")
        if(NOT DEP_SOURCE_ONLY)
            add_subdirectory("${_source}" "${_deps_build}")
        else()
            set(MBOOTCORE_DEP_${_name}_SOURCE_DIR "${_source}" PARENT_SCOPE)
            set(MBOOTCORE_DEP_${_name}_BUILD_DIR "${_deps_build}" PARENT_SCOPE)
        endif()
        return()
    endif()

    # ------------------------------------------------------------------
    # Version changed: clear stale cache.
    # ------------------------------------------------------------------
    if(_source_valid AND NOT _cached_version STREQUAL _version)
        message(STATUS "[deps] ${_name}: version changed "
            "${_cached_version} -> ${_version}, updating")
        file(REMOVE_RECURSE "${_source}")
        file(REMOVE "${_stamp}")
    endif()

    # ------------------------------------------------------------------
    # Case 2: archive cache exists and hash matches → extract and build.
    # ------------------------------------------------------------------
    if(_existing_hash STREQUAL _sha256)
        message(STATUS "[deps] ${_name}: archive verified, extracting")
        file(MAKE_DIRECTORY "${_source}")
        # Use execute_process to suppress harmless symlink warnings on Windows
        # (e.g. mbedtls contains ~147 symlinks in ML-DSA examples).
        execute_process(
            COMMAND ${CMAKE_COMMAND} -E tar xf "${_cache_file}"
            WORKING_DIRECTORY "${_source}"
            OUTPUT_QUIET
            ERROR_VARIABLE _extract_errors
            RESULT_VARIABLE _extract_result
        )
        if(NOT _extract_result EQUAL 0)
            message(FATAL_ERROR
                "[deps] ${_name}: extraction failed (exit code ${_extract_result})\n"
                "  ${_extract_errors}")
        endif()
        # GitHub archives wrap in a top-level dir; flatten.
        _mbootcore_flatten_extraction("${_source}")
        file(WRITE "${_stamp}" "${_version}\n")
        message(STATUS "[deps] ${_name}: extracted to source, adding to build")
        if(NOT DEP_SOURCE_ONLY)
            add_subdirectory("${_source}" "${_deps_build}")
        else()
            set(MBOOTCORE_DEP_${_name}_SOURCE_DIR "${_source}" PARENT_SCOPE)
            set(MBOOTCORE_DEP_${_name}_BUILD_DIR "${_deps_build}" PARENT_SCOPE)
        endif()
        return()
    endif()

    # ------------------------------------------------------------------
    # Archive hash mismatch or missing: re-download.
    # ------------------------------------------------------------------
    file(REMOVE "${_cache_file}")

    message(STATUS "[deps] ${_name}: downloading ${_url}")
    file(DOWNLOAD
        "${_url}"
        "${_cache_file}"
        INACTIVITY_TIMEOUT 60
        SHOW_PROGRESS
        TLS_VERIFY ON
        STATUS _dl_status
    )
    list(GET _dl_status 0 _dl_result)
    if(NOT _dl_result EQUAL 0)
        list(GET _dl_status 1 _dl_error)
        message(STATUS "[deps] ${_name}: TLS verify failed, retrying without TLS verification")
        file(REMOVE "${_cache_file}")
        file(DOWNLOAD
            "${_url}"
            "${_cache_file}"
            INACTIVITY_TIMEOUT 60
            SHOW_PROGRESS
            TLS_VERIFY OFF
            STATUS _dl_status2
        )
        list(GET _dl_status2 0 _dl_result2)
        if(NOT _dl_result2 EQUAL 0)
            list(GET _dl_status2 1 _dl_error2)
            file(REMOVE "${_cache_file}")
            message(FATAL_ERROR
                "[deps] ${_name}: download failed (${_dl_error2})\n"
                "  URL: ${_url}\n"
                "  Download this file manually into ${_cache_file} or check your connection."
            )
        endif()
    endif()

    # Verify SHA256 after successful download.
    file(SHA256 "${_cache_file}" _dl_hash)
    string(TOLOWER "${_dl_hash}" _dl_hash_lower)
    string(TOLOWER "${_sha256}" _sha256_lower)
    if(NOT _dl_hash_lower STREQUAL _sha256_lower)
        file(REMOVE "${_cache_file}")
        message(FATAL_ERROR
            "[deps] ${_name}: SHA256 mismatch after download\n"
            "  Expected: ${_sha256}\n"
            "  Got:      ${_dl_hash}"
        )
    endif()
    message(STATUS "[deps] ${_name}: download complete (SHA256 verified)")

    # Extract into source.
    file(MAKE_DIRECTORY "${_source}")
    # Use execute_process to suppress harmless symlink warnings on Windows.
    execute_process(
        COMMAND ${CMAKE_COMMAND} -E tar xf "${_cache_file}"
        WORKING_DIRECTORY "${_source}"
        OUTPUT_QUIET
        ERROR_VARIABLE _extract_errors
        RESULT_VARIABLE _extract_result
    )
    if(NOT _extract_result EQUAL 0)
        message(FATAL_ERROR
            "[deps] ${_name}: extraction failed (exit code ${_extract_result})\n"
            "  ${_extract_errors}")
    endif()
    _mbootcore_flatten_extraction("${_source}")
    file(WRITE "${_stamp}" "${_version}\n")

    message(STATUS "[deps] ${_name}: adding to build")
    if(NOT DEP_SOURCE_ONLY)
        add_subdirectory("${_source}" "${_deps_build}")
    else()
        set(MBOOTCORE_DEP_${_name}_SOURCE_DIR "${_source}" PARENT_SCOPE)
        set(MBOOTCORE_DEP_${_name}_BUILD_DIR "${_deps_build}" PARENT_SCOPE)
    endif()
endfunction()

# ---------------------------------------------------------------------------
# _mbootcore_flatten_extraction
#
# GitHub archives extract to <repo>-<tag>/ internally.  This helper
# detects that single top-level dir and moves its contents up, so the
# CMakeLists.txt is directly in source/.
# ---------------------------------------------------------------------------
function(_mbootcore_flatten_extraction BASE_DIR)
    set(_subdirs)
    file(GLOB _entries RELATIVE "${BASE_DIR}" "${BASE_DIR}/*")
    foreach(_e ${_entries})
        if(IS_DIRECTORY "${BASE_DIR}/${_e}")
            list(APPEND _subdirs "${_e}")
        endif()
    endforeach()

    list(LENGTH _subdirs _subdir_count)
    if(_subdir_count EQUAL 1)
        list(GET _subdirs 0 _single)
        if(NOT EXISTS "${BASE_DIR}/CMakeLists.txt")
            # Clean any stale temp dir from a previous interrupted run.
            file(REMOVE_RECURSE "${BASE_DIR}/_tmp_mv")
            # Atomically move the single wrapper directory contents up.
            file(RENAME "${BASE_DIR}/${_single}" "${BASE_DIR}/_tmp_mv")
            file(GLOB _tmp_files "${BASE_DIR}/_tmp_mv/*")
            file(COPY ${_tmp_files} DESTINATION "${BASE_DIR}")
            file(REMOVE_RECURSE "${BASE_DIR}/_tmp_mv")
        endif()
    endif()
endfunction()

# ---------------------------------------------------------------------------
# mbootcore_clean_dependency
# ---------------------------------------------------------------------------
function(mbootcore_clean_dependency NAME)
    file(REMOVE_RECURSE "${CMAKE_SOURCE_DIR}/deps/${NAME}")
    message(STATUS "[deps] ${NAME}: cache cleared")
endfunction()

# ---------------------------------------------------------------------------
# mbootcore_clean_all_deps
# ---------------------------------------------------------------------------
function(mbootcore_clean_all_deps)
    file(GLOB _entries "${CMAKE_SOURCE_DIR}/deps/*")
    foreach(_e ${_entries})
        if(IS_DIRECTORY "${_e}")
            file(REMOVE_RECURSE "${_e}")
        endif()
    endforeach()
    message(STATUS "[deps] all dependency caches cleared")
endfunction()
