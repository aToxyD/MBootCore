# ==========================================================================
# tests/cmake/ExportValidation.cmake
# --------------------------------------------------------------------------
# Export Validation Gate — 3 hermetic regression tests that prevent
# dependency leaks and verify real-world package consumption.
#
# Gate 1: Export Integrity      — static inspection of MBootCoreTargets.cmake
# Gate 2: Consumer Package Test — real find_package + build + API usage
# Gate 3: Install Tree Audit    — architectural contract enforcement
#
# Run via:  cmake -DBUILD_DIR=<build> -P tests/cmake/ExportValidation.cmake
# ==========================================================================

cmake_minimum_required(VERSION 3.20)

if(NOT DEFINED BUILD_DIR)
    message(FATAL_ERROR "BUILD_DIR not defined — pass via -DBUILD_DIR=<path>")
endif()

set(EXIT_CODE 0)

if(DEFINED ENV{TMPDIR})
    set(_tmp_root "$ENV{TMPDIR}")
elseif(DEFINED ENV{TEMP})
    set(_tmp_root "$ENV{TEMP}")
else()
    set(_tmp_root "/tmp")
endif()
string(TIMESTAMP _ts "%Y%m%d-%H%M%S")
string(RANDOM LENGTH 6 ALPHABET 0123456789ABCDEF _rnd)
set(_tmp_base "${_tmp_root}/mbootcore-export-${_ts}-${_rnd}")
if(EXISTS "${_tmp_base}")
    file(REMOVE_RECURSE "${_tmp_base}")
endif()
file(MAKE_DIRECTORY "${_tmp_base}")
set(INSTALL_PREFIX "${_tmp_base}/install")

# ==========================================================================
# Gate 1: Export Integrity Test (fast static inspection)
# ==========================================================================
message(STATUS "")
message(STATUS "=== Gate 1: Export Integrity Test ===")

file(GLOB_RECURSE _targets_files "${BUILD_DIR}/CMakeFiles/Export/*/MBootCoreTargets.cmake")
if(NOT _targets_files)
    message(STATUS "[FAIL] Gate 1: MBootCoreTargets.cmake not found in build tree")
    set(EXIT_CODE 1)
else()
    list(GET _targets_files 0 _targets_file)
    file(READ "${_targets_file}" _targets_content)

    set(_FORBIDDEN_NAMES ZLIB:: MbedTLS:: tfpsacrypto usb-1.0 nlohmann_json)
    set(_found_leak FALSE)
    foreach(_name IN LISTS _FORBIDDEN_NAMES)
        string(FIND "${_targets_content}" "${_name}" _pos)
        if(NOT _pos EQUAL -1)
            message(STATUS "[FAIL] Gate 1: found '${_name}' in MBootCoreTargets.cmake")
            set(_found_leak TRUE)
        endif()
    endforeach()

    if(_found_leak)
        set(EXIT_CODE 1)
        message(STATUS "[FAIL] Gate 1: Export Integrity Test FAILED")
    else()
        message(STATUS "[PASS] Gate 1: Export Integrity Test passed")
    endif()
endif()

# ==========================================================================
# Gate 1b: Link Interface Validation (Windows system library ownership)
# Validated inside the consumer project via get_target_property on the
# imported MBootCore::mbootcore target.
# ==========================================================================

# ==========================================================================
# Gate 2: Consumer Package Test (functional, hermetic)
# ==========================================================================
message(STATUS "")
message(STATUS "=== Gate 2: Consumer Package Test ===")

set(_gate2_passed FALSE)

# Install to isolated prefix (best-effort: mboot-studio may fail but
# mbootcore + headers + cmake config are installed before that target)
file(REMOVE_RECURSE "${INSTALL_PREFIX}")
execute_process(
    COMMAND ${CMAKE_COMMAND} --install "${BUILD_DIR}" --prefix "${INSTALL_PREFIX}"
    RESULT_VARIABLE _install_result
    OUTPUT_VARIABLE _install_out
    ERROR_VARIABLE  _install_err
)

# Verify key artifacts exist regardless of install exit code
if(NOT EXISTS "${INSTALL_PREFIX}/lib/cmake/MBootCore/MBootCoreConfig.cmake")
    message(STATUS "[FAIL] Gate 2: MBootCoreConfig.cmake not found in install tree")
else()
    # Create minimal consumer project
    set(_consumer_dir "${_tmp_base}/consumer")
    file(REMOVE_RECURSE "${_consumer_dir}")
    file(MAKE_DIRECTORY "${_consumer_dir}")

    set(_cmake_content "cmake_minimum_required(VERSION 3.20)\n")
    string(APPEND _cmake_content "project(ConsumerTest CXX)\n")
    string(APPEND _cmake_content "find_package(MBootCore REQUIRED)\n")
    string(APPEND _cmake_content "add_executable(consumer_app main.cpp)\n")
    string(APPEND _cmake_content "target_link_libraries(consumer_app PRIVATE MBootCore::mbootcore)\n")
    if(WIN32)
        string(APPEND _cmake_content "# Gate 1b: validate exported link interface\n")
        string(APPEND _cmake_content "get_target_property(_links MBootCore::mbootcore INTERFACE_LINK_LIBRARIES)\n")
        string(APPEND _cmake_content "if(NOT _links)\n")
        string(APPEND _cmake_content "    message(FATAL_ERROR \"MBootCore::mbootcore has no INTERFACE_LINK_LIBRARIES\")\n")
        string(APPEND _cmake_content "endif()\n")
        string(APPEND _cmake_content "foreach(_lib IN ITEMS setupapi ws2_32 bcrypt)\n")
        string(APPEND _cmake_content "    list(FIND _links \"\${_lib}\" _pos)\n")
        string(APPEND _cmake_content "    if(_pos EQUAL -1)\n")
        string(APPEND _cmake_content "        message(FATAL_ERROR \"'\${_lib}' not in MBootCore::mbootcore INTERFACE_LINK_LIBRARIES\")\n")
        string(APPEND _cmake_content "    endif()\n")
        string(APPEND _cmake_content "endforeach()\n")
        string(APPEND _cmake_content "message(STATUS \"Link interface: \${_links}\")\n")
    endif()
    file(WRITE "${_consumer_dir}/CMakeLists.txt" "${_cmake_content}")

    set(_main_content "#include <mbootcore/transport/network/makeTcpBackend.hpp>\n")
    string(APPEND _main_content "#include <mbootcore/security/MbedTLSHashProvider.hpp>\n")
    string(APPEND _main_content "\n")
    string(APPEND _main_content "int main() {\n")
    string(APPEND _main_content "    // TCP path — forces WinSockTcpBackend.o -> ws2_32\n")
    string(APPEND _main_content "    auto tcp = mbootcore::transport::network::makeTcpBackend();\n")
    string(APPEND _main_content "    (void)tcp;\n")
    string(APPEND _main_content "\n")
    string(APPEND _main_content "    // Crypto path — exercises the public hash provider and verifies that\n")
    string(APPEND _main_content "    // the exported Windows crypto link interface is complete.\n")
    string(APPEND _main_content "    auto hash = mbootcore::security::makeMbedTLSHashProvider();\n")
    string(APPEND _main_content "    (void)hash;\n")
    string(APPEND _main_content "\n")
    string(APPEND _main_content "    return 0;\n")
    string(APPEND _main_content "}\n")
    file(WRITE "${_consumer_dir}/main.cpp" "${_main_content}")

    # Configure
    execute_process(
        COMMAND ${CMAKE_COMMAND}
            -S "${_consumer_dir}"
            -B "${_consumer_dir}/build"
            -DMBootCore_DIR=${INSTALL_PREFIX}/lib/cmake/MBootCore
            -DCMAKE_BUILD_TYPE=Debug
        RESULT_VARIABLE _cfg_result
        OUTPUT_VARIABLE _cfg_out
        ERROR_VARIABLE  _cfg_err
    )

    if(NOT _cfg_result EQUAL 0)
        message(STATUS "[FAIL] Gate 2: consumer configure failed")
        message(STATUS "  ${_cfg_err}")
    else()
        # Verify consumer cache has no references to the producer build/source tree
        set(_cache_file "${_consumer_dir}/build/CMakeCache.txt")
        if(NOT EXISTS "${_cache_file}")
            message(STATUS "[FAIL] Gate 2: CMakeCache.txt was not generated")
            set(_cfg_result 1)
        else()
            file(READ "${_cache_file}" _cache_content)
            set(_forbidden_paths
                "${BUILD_DIR}"
                "${CMAKE_BINARY_DIR}"
                "${CMAKE_SOURCE_DIR}"
            )
            set(_cache_leak FALSE)
            foreach(_fp IN LISTS _forbidden_paths)
                string(FIND "${_cache_content}" "${_fp}" _fp_pos)
                if(NOT _fp_pos EQUAL -1)
                    message(STATUS "[FAIL] Gate 2: consumer cache references producer path: ${_fp}")
                    set(_cache_leak TRUE)
                endif()
            endforeach()
            if(_cache_leak)
                set(_cfg_result 1)
            endif()
        endif()
    endif()

    # Build + link (only if configure and cache check both passed)
    if(_cfg_result EQUAL 0)
        execute_process(
            COMMAND ${CMAKE_COMMAND} --build "${_consumer_dir}/build"
            RESULT_VARIABLE _bld_result
            OUTPUT_VARIABLE _bld_out
            ERROR_VARIABLE  _bld_err
        )

        if(NOT _bld_result EQUAL 0)
            message(STATUS "[FAIL] Gate 2: consumer build+link failed")
            message(STATUS "  ${_bld_err}")
        else()
            # Verify no extra find_package was needed
            string(FIND "${_cfg_out}" "Could not find package" _miss_pos)
            if(NOT _miss_pos EQUAL -1)
                message(STATUS "[FAIL] Gate 2: missing dependency detected during configure")
            else()
                message(STATUS "[PASS] Gate 2: Consumer Package Test passed")
                set(_gate2_passed TRUE)
            endif()
        endif()
    endif()

endif()

if(NOT _gate2_passed)
    set(EXIT_CODE 1)
endif()

# ==========================================================================
# Gate 3: Install Tree Audit (architectural contract enforcement)
# ==========================================================================
message(STATUS "")
message(STATUS "=== Gate 3: Install Tree Audit ===")

set(_audit_failed FALSE)

if(NOT DEFINED CMAKE_INSTALL_DOCDIR)
    set(CMAKE_INSTALL_DOCDIR "share/doc/${PROJECT_NAME}")
endif()

# --- Forbidden Categories ---
# Third-party headers
foreach(_p IN ITEMS include/mbedtls include/nlohmann include/libusb-1.0)
    file(GLOB _matches "${INSTALL_PREFIX}/${_p}/*")
    if(_matches)
        message(STATUS "[FAIL] Gate 3: forbidden category present: ${_p}/")
        set(_audit_failed TRUE)
    endif()
endforeach()

# Third-party archives
foreach(_p IN ITEMS "lib/libz*" "lib/libmbedtls*" "lib/libmbedx509*" "lib/libmbedcrypto*" "lib/libusb*")
    file(GLOB _matches "${INSTALL_PREFIX}/${_p}")
    foreach(_m IN LISTS _matches)
        if(EXISTS "${_m}" AND NOT IS_DIRECTORY "${_m}" AND _m MATCHES "\\.a$")
            message(STATUS "[FAIL] Gate 3: forbidden archive: ${_m}")
            set(_audit_failed TRUE)
        endif()
    endforeach()
endforeach()

# Third-party package configs
foreach(_p IN ITEMS lib/cmake/zlib lib/cmake/libusb lib/cmake/nlohmann_json)
    if(EXISTS "${INSTALL_PREFIX}/${_p}")
        message(STATUS "[FAIL] Gate 3: forbidden package config: ${_p}/")
        set(_audit_failed TRUE)
    endif()
endforeach()

# --- Required Files ---
foreach(_p IN ITEMS
    "lib/libmbootcore.a"
    "include/mbootcore"
    "include/sdk"
    "lib/cmake/MBootCore"
    "${CMAKE_INSTALL_DOCDIR}/LICENSE"
    "${CMAKE_INSTALL_DOCDIR}/NOTICE"
    "${CMAKE_INSTALL_DOCDIR}/ThirdPartyLicenses.md"
)
    if(NOT EXISTS "${INSTALL_PREFIX}/${_p}")
        message(STATUS "[FAIL] Gate 3: required path missing: ${_p}")
        set(_audit_failed TRUE)
    endif()
endforeach()

if(_audit_failed)
    set(EXIT_CODE 1)
    message(STATUS "[FAIL] Gate 3: Install Tree Audit FAILED")
else()
    message(STATUS "[PASS] Gate 3: Install Tree Audit passed")
endif()

# ==========================================================================
# Cleanup — preserve on failure for debugging
# ==========================================================================
message(STATUS "")
if(EXIT_CODE EQUAL 0)
    file(REMOVE_RECURSE "${_tmp_base}")
    message(STATUS "=== All Export Validation Gates passed ===")
else()
    message(STATUS "")
    message(STATUS "Temp directory preserved for debugging: ${_tmp_base}")
    message(STATUS "  Key files to inspect:")
    message(STATUS "    ${_tmp_base}/consumer/build/CMakeCache.txt")
    message(STATUS "    ${_tmp_base}/consumer/build/CMakeFiles/")
    message(STATUS "    ${_tmp_base}/install/lib/cmake/MBootCore/MBootCoreTargets.cmake")
    message(FATAL_ERROR "=== Export Validation FAILED ===")
endif()
