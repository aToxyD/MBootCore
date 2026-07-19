# ==========================================================================
# DependencyManifest.cmake
# --------------------------------------------------------------------------
# Abstract dependency manifest.  Defines each third-party dependency via
# the mbootcore_dependency() macro.  DependencyManager.cmake reads this
# manifest and decides whether to download, use cache, or build.
#
# Adding a new dependency: just add one mbootcore_dependency() call here.
# ==========================================================================

include_guard()

# ---------------------------------------------------------------------------
# mbootcore_dependency
#
# Macro that records a dependency entry in a global list for later
# processing by DependencyManager.cmake.
# ---------------------------------------------------------------------------
macro(mbootcore_dependency)
    cmake_parse_arguments(DEP
        ""
        "NAME;VERSION;URL;SHA256"
        ""
        ${ARGN}
    )
    if(NOT DEP_NAME OR NOT DEP_VERSION OR NOT DEP_URL OR NOT DEP_SHA256)
        message(FATAL_ERROR
            "mbootcore_dependency: NAME, VERSION, URL, and SHA256 are required\n"
            "  Received: NAME=${DEP_NAME} VERSION=${DEP_VERSION} URL=${DEP_URL} SHA256=${DEP_SHA256}"
        )
    endif()

    list(APPEND MBOOTCORE_DEPENDENCIES "${DEP_NAME}")
    set(MBOOTCORE_DEP_${DEP_NAME}_NAME    "${DEP_NAME}")
    set(MBOOTCORE_DEP_${DEP_NAME}_VERSION "${DEP_VERSION}")
    set(MBOOTCORE_DEP_${DEP_NAME}_URL     "${DEP_URL}")
    set(MBOOTCORE_DEP_${DEP_NAME}_SHA256  "${DEP_SHA256}")
endmacro()

# ---------------------------------------------------------------------------
# Dependency manifest entries
# ---------------------------------------------------------------------------

mbootcore_dependency(
    NAME    nlohmann_json
    VERSION ${MBOOTCORE_JSON_VERSION}
    URL     ${MBOOTCORE_JSON_GITHUB_URL}
    SHA256  ${MBOOTCORE_JSON_SHA256}
)

mbootcore_dependency(
    NAME    zlib
    VERSION ${MBOOTCORE_ZLIB_VERSION}
    URL     ${MBOOTCORE_ZLIB_GITHUB_URL}
    SHA256  ${MBOOTCORE_ZLIB_SHA256}
)

mbootcore_dependency(
    NAME    mbedtls
    VERSION ${MBOOTCORE_MBEDTLS_VERSION}
    URL     ${MBOOTCORE_MBEDTLS_GITHUB_URL}
    SHA256  ${MBOOTCORE_MBEDTLS_SHA256}
)

mbootcore_dependency(
    NAME    libusb
    VERSION ${MBOOTCORE_LIBUSB_VERSION}
    URL     ${MBOOTCORE_LIBUSB_GITHUB_URL}
    SHA256  ${MBOOTCORE_LIBUSB_SHA256}
)

mbootcore_dependency(
    NAME    Catch2
    VERSION ${MBOOTCORE_CATCH2_VERSION}
    URL     ${MBOOTCORE_CATCH2_GITHUB_URL}
    SHA256  ${MBOOTCORE_CATCH2_SHA256}
)
