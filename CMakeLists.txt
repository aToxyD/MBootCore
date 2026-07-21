cmake_minimum_required(VERSION 3.20)

# Version — single source of truth (VERSION file).
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/VERSION" _MBOOTCORE_RAW_VERSION)
string(STRIP "${_MBOOTCORE_RAW_VERSION}" _MBOOTCORE_RAW_VERSION)
if(NOT _MBOOTCORE_RAW_VERSION MATCHES "^[0-9]+\\.[0-9]+\\.[0-9]+$")
    message(FATAL_ERROR
        "Invalid VERSION file content: '${_MBOOTCORE_RAW_VERSION}'\n"
        "Expected format: X.Y.Z (e.g. 1.0.1)")
endif()

project(MBootCore VERSION "${_MBOOTCORE_RAW_VERSION}" LANGUAGES CXX)

# Explicit policy settings to suppress warnings on CMake >= 3.30
if(POLICY CMP0156)
    cmake_policy(SET CMP0156 NEW)
endif()
if(POLICY CMP0167)
    cmake_policy(SET CMP0167 NEW)
endif()
if(POLICY CMP0177)
    cmake_policy(SET CMP0177 NEW)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# MinGW: statically link GCC and C++ runtimes so executables have zero DLL dependencies.
# -static-libgcc and -static-libstdc++ are GCC driver flags that work regardless of position.
# -static tells the GCC driver to pass -Bstatic to ld (instead of -Bdynamic). This makes the
# driver's implicit -lpthread resolve to libwinpthread.a (real static archive with actual code)
# instead of libwinpthread.dll.a (DLL import). System DLLs (-lsetupapi, -lws2_32, etc.) still
# resolve correctly because MinGW provides .a files for them that contain COFF import objects.
if(MINGW)
    add_link_options(-static-libgcc -static-libstdc++ -static)

    # Compatibility shim.
    #
    # Static MinGW runtime linking is handled by the global linker options
    # above. This function intentionally remains a no-op so existing call
    # sites remain platform-independent.
    function(mbootcore_static_winpthread target)
    endfunction()
else()
    function(mbootcore_static_winpthread target)
    endfunction()
endif()

include(GNUInstallDirs)

# ---------------------------------------------------------------------------
# Platform & Architecture — single point of truth for SDK Distribution.
# ---------------------------------------------------------------------------
if(WIN32)
    if(MSVC)
        set(MBOOTCORE_PLATFORM_NAME "windows-msvc")
    else()
        set(MBOOTCORE_PLATFORM_NAME "windows-mingw")
    endif()
elseif(APPLE)
    set(MBOOTCORE_PLATFORM_NAME "macos-clang")
elseif(UNIX)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        set(MBOOTCORE_PLATFORM_NAME "linux-gcc")
    else()
        set(MBOOTCORE_PLATFORM_NAME "linux-clang")
    endif()
else()
    set(MBOOTCORE_PLATFORM_NAME "unknown")
endif()

# Architecture detection — ABI-based (compiler output, not host CPU).
# ARM detected via CMAKE_SYSTEM_PROCESSOR; x86/x64 via CMAKE_SIZEOF_VOID_P.
string(TOLOWER "${CMAKE_SYSTEM_PROCESSOR}" _proc)
if(_proc MATCHES "^(arm64|aarch64)$")
    set(MBOOTCORE_ARCH "arm64")
elseif(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(MBOOTCORE_ARCH "x64")
elseif(CMAKE_SIZEOF_VOID_P EQUAL 4)
    set(MBOOTCORE_ARCH "x86")
else()
    set(MBOOTCORE_ARCH "${_proc}")
endif()

# Version — alias from project() VERSION.
set(MBOOTCORE_VERSION "${PROJECT_VERSION}")

# Build metadata — consumed by release/SDKDistribution.cmake.
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/BuildMetadata.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/cmake/MBootCoreBuildInfo.cmake
    @ONLY
)

# ---------------------------------------------------------------------------
# Compiler Warnings — centralized warning policy for all MBootCore targets.
# See cmake/CompilerWarnings.cmake for details.
# ---------------------------------------------------------------------------
include(cmake/CompilerWarnings.cmake)

# ---------------------------------------------------------------------------
# Sanitizers — optional AddressSanitizer and UndefinedBehaviorSanitizer.
# See cmake/Sanitizers.cmake for details.
# ---------------------------------------------------------------------------
include(cmake/Sanitizers.cmake)

# ---------------------------------------------------------------------------
# Dependency Manager — deterministic, self-contained dependency management.
# See docs/guides/DependencyManager.md for details.
# ---------------------------------------------------------------------------
include(cmake/DependencyManager.cmake)
include(cmake/MonolithicArchive.cmake)

add_subdirectory(lib)
add_subdirectory(sdk)

# ---------------------------------------------------------------------------
# Quality Gates — configure-time validations for forbidden patterns,
# deprecated headers, placeholder implementations, etc.
# ---------------------------------------------------------------------------
include(cmake/QualityGates.cmake)

option(MBOOTCORE_BUILD_TESTS "Build tests" ON)

option(MBOOTCORE_BUILD_SECURITY_TESTS
    "Build security robustness tests and parser regression corpus"
    ON)

option(MBOOTCORE_BUILD_FUZZERS
    "Build coverage-guided fuzz harnesses (requires libFuzzer-capable compiler)"
    OFF)

option(MBOOTCORE_BUILD_TOOLS "Build developer tools (PluginWizard, DocGenerator)" ON)

# ---------------------------------------------------------------------------
# Build-Option Dependency Validation
# ---------------------------------------------------------------------------
# SDK tests (tests/sdk/) link against tool libraries (mboot-doc-generator,
# mboot-plugin-wizard).  Enabling tests without tools would produce a
# confusing linker error.  Fail early at configure time instead.
# ---------------------------------------------------------------------------
if(MBOOTCORE_BUILD_TESTS AND NOT MBOOTCORE_BUILD_TOOLS)
    message(FATAL_ERROR
        "MBOOTCORE_BUILD_TESTS=ON requires MBOOTCORE_BUILD_TOOLS=ON.\n"
        "SDK tests (sdk_doc_generator_tool_test, sdk_plugin_wizard_test) link "
        "against tool libraries (mboot-doc-generator, mboot-plugin-wizard).\n\n"
        "Fix: add -DMBOOTCORE_BUILD_TOOLS=ON to your cmake command, or "
        "disable tests with -DMBOOTCORE_BUILD_TESTS=OFF."
    )
endif()

# Tools must be processed before tests — SDK tests link against tool libraries.
if(MBOOTCORE_BUILD_TOOLS)
    add_subdirectory(tools)
endif()

if(MBOOTCORE_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

option(MBOOTCORE_BUILD_STUDIO
    "Build the Qt GUI application (mboot-studio)"
    ON)
option(MBOOTCORE_BUILD_CLI "Build CLI application" ON)

find_package(Qt6 QUIET COMPONENTS Core Widgets)

if(MBOOTCORE_BUILD_STUDIO AND NOT Qt6_FOUND)
    message(STATUS
        "Qt6 was not found.  mboot-studio will be disabled automatically.")
    set(MBOOTCORE_BUILD_STUDIO OFF CACHE BOOL "" FORCE)
endif()

if(MBOOTCORE_BUILD_CLI)
    add_subdirectory(apps)
endif()

option(MBOOTCORE_BUILD_EXAMPLES "Build examples" OFF)
if(MBOOTCORE_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# Optional Doxygen documentation generation
find_package(Doxygen QUIET)
if(DOXYGEN_FOUND)
    configure_file(
        ${CMAKE_SOURCE_DIR}/docs/Doxyfile.in
        ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
        @ONLY
    )
    add_custom_target(docs
        COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Generating API documentation with Doxygen"
        VERBATIM
    )
    add_custom_target(docs-open
        COMMAND ${CMAKE_COMMAND} -E echo "Open ${CMAKE_CURRENT_BINARY_DIR}/docs/api/html/index.html in your browser"
        DEPENDS docs
    )
else()
    message(STATUS "Documentation disabled (Doxygen not found)")
endif()

# Install rules
include(cmake/InstallRules.cmake)

# Call install functions
install_mbootcore_targets()
install_mbootcore_headers()
install_mbootsdk_headers()
install_mbootcore_examples()
install_mbootcore_docs()

# Legal: license notices (required when bundling third-party dependencies)
install(FILES
    ${CMAKE_SOURCE_DIR}/LICENSE
    ${CMAKE_SOURCE_DIR}/NOTICE
    ${CMAKE_SOURCE_DIR}/docs/reference/ThirdPartyLicenses.md
    ${CMAKE_SOURCE_DIR}/VERSION
    DESTINATION ${CMAKE_INSTALL_DOCDIR}
)

# Version configuration — read from file (single source of truth)
file(READ "${CMAKE_SOURCE_DIR}/SDK_VERSION" MBOOTCORE_SDK_VERSION)
string(STRIP "${MBOOTCORE_SDK_VERSION}" MBOOTCORE_SDK_VERSION)

# Configure CMake package config file from template
include(CMakePackageConfigHelpers)
configure_package_config_file(
    ${CMAKE_SOURCE_DIR}/cmake/MBootCoreConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/cmake/MBootCoreConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/MBootCore
    PATH_VARS CMAKE_INSTALL_INCLUDEDIR CMAKE_INSTALL_LIBDIR
)

# Also install the configured files
configure_file(
    ${CMAKE_SOURCE_DIR}/cmake/MBootCoreConfigVersion.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/cmake/MBootCoreConfigVersion.cmake
    @ONLY
)
install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/cmake/MBootCoreConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/cmake/MBootCoreConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/MBootCore
)

# CPack packaging
include(cmake/MBootCoreCPack.cmake)
include(CPack)
