set(CPACK_PACKAGE_NAME "MBootCore")
set(CPACK_PACKAGE_VENDOR "MBootCore Team")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Professional low-level C++17 Framework for BootROM protocols")
set(CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${PROJECT_VERSION_PATCH})
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "MBootCore-${PROJECT_VERSION}")
set(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_SOURCE_DIR}/README.md")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE")

# -- Install prefix inside packages (not the build machine) ------------------
set(CPACK_PACKAGING_INSTALL_PREFIX "/opt/MBootCore")
set(CPACK_PACKAGE_INSTALL_PREFIX "/opt/MBootCore")

# -- Archive generators (all platforms) ---------------------------------------
set(CPACK_GENERATOR "ZIP")
set(CPACK_ARCHIVE_COMPONENT_INSTALL OFF)

# -- Platform-specific native package generators ------------------------------
if(UNIX AND NOT APPLE)
    # ── Linux: DEB ─────────────────────────────────────────────────────
    set(CPACK_GENERATOR "${CPACK_GENERATOR};DEB")

    execute_process(
        COMMAND dpkg --print-architecture
        OUTPUT_VARIABLE _mboot_deb_arch
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
    if(NOT _mboot_deb_arch)
        set(_mboot_deb_arch "amd64")
    endif()

    # DEB metadata
    set(CPACK_DEBIAN_PACKAGE_NAME "mbootcore")
    set(CPACK_DEBIAN_PACKAGE_SECTION "devel")
    set(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")
    set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "${_mboot_deb_arch}")
    set(CPACK_DEBIAN_PACKAGE_HOMEPAGE "https://github.com/aToxyD/MBootCore")
    unset(CPACK_DEBIAN_PACKAGE_DEPENDS)
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER "MBootCore Team")
    set(CPACK_DEBIAN_PACKAGE_DESCRIPTION
        "Professional low-level C++17 framework for BootROM protocols.")
endif()

