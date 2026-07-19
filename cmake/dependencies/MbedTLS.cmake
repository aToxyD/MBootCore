# ==========================================================================
# dependencies/MbedTLS.cmake
# --------------------------------------------------------------------------
# Cryptographic backend.  Conditional on MBOOTCORE_ENABLE_CRYPTO.
# When OFF, all crypto returns NotSupported (no download).
#
# Mbed TLS uses CMake natively (add_subdirectory works directly).
# No Perl, no ExternalProject_Add, no host dependencies.
# ==========================================================================

message(STATUS "")

if(MBOOTCORE_ENABLE_CRYPTO)
    mbootcore_require_dependency(NAME mbedtls SOURCE_ONLY)

    set(MBOOTCORE_MBEDTLS_SOURCE_DIR "${CMAKE_SOURCE_DIR}/deps/mbedtls/source")
    set(MBOOTCORE_MBEDTLS_BUILD_DIR  "${CMAKE_BINARY_DIR}/_deps_build/mbedtls")

    # Patch mbedtls source to:
    # 1. Suppress cmake_minimum_required(VERSION 3.5.1) deprecation (CMake 3.31+)
    # 2. Remove Python find_package — MBootCore is a pure C++ project
    foreach(_cml IN ITEMS
        "${MBOOTCORE_MBEDTLS_SOURCE_DIR}/CMakeLists.txt"
        "${MBOOTCORE_MBEDTLS_SOURCE_DIR}/tf-psa-crypto/CMakeLists.txt"
    )
        if(EXISTS "${_cml}")
            file(READ "${_cml}" _cml_content)
            string(REPLACE
                "cmake_minimum_required(VERSION 3.5.1)"
                "cmake_minimum_required(VERSION 3.5...3.10)"
                _cml_content "${_cml_content}")
            # Remove unconditional Python detection — not needed when
            # ENABLE_TESTING=OFF and ENABLE_PROGRAMS=OFF.
            string(REPLACE
                [[# Python 3 is only needed here to check for configuration warnings.
if(NOT CMAKE_VERSION VERSION_LESS 3.15.0)
    set(Python3_FIND_STRATEGY LOCATION)
    find_package(Python3 COMPONENTS Interpreter)
    if(Python3_Interpreter_FOUND)
        set(MBEDTLS_PYTHON_EXECUTABLE ${Python3_EXECUTABLE})
    endif()
else()
    find_package(PythonInterp 3)
    if(PYTHONINTERP_FOUND)
        set(MBEDTLS_PYTHON_EXECUTABLE ${PYTHON_EXECUTABLE})
    endif()
endif()]]
                "# Python detection removed — not needed (ENABLE_TESTING=OFF)."
                _cml_content "${_cml_content}")
            string(REPLACE
                [[# Python 3 is only needed here to check for configuration warnings.
if(NOT CMAKE_VERSION VERSION_LESS 3.15.0)
    set(Python3_FIND_STRATEGY LOCATION)
    find_package(Python3 COMPONENTS Interpreter)
    if(Python3_Interpreter_FOUND)
        set(TF_PSA_CRYPTO_PYTHON_EXECUTABLE ${Python3_EXECUTABLE})
    endif()
else()
    find_package(PythonInterp 3)
    if(PYTHONINTERP_FOUND)
        set(TF_PSA_CRYPTO_PYTHON_EXECUTABLE ${PYTHON_EXECUTABLE})
    endif()
endif()]]
                "# Python detection removed — not needed (ENABLE_TESTING=OFF)."
                _cml_content "${_cml_content}")
            file(WRITE "${_cml}" "${_cml_content}")
        endif()
    endforeach()

    # Disable mbedtls test suites and example programs — we only need the
    # crypto libraries.  This eliminates the Python dependency entirely
    # (Python is only required by mbedtls's own test infrastructure).
    set(ENABLE_TESTING OFF CACHE BOOL "" FORCE)
    set(ENABLE_PROGRAMS OFF CACHE BOOL "" FORCE)
    set(INSTALL_MBEDTLS_HEADERS OFF CACHE BOOL "" FORCE)
    set(DISABLE_PACKAGE_CONFIG_AND_INSTALL ON CACHE BOOL "" FORCE)

    add_subdirectory("${MBOOTCORE_MBEDTLS_SOURCE_DIR}" "${MBOOTCORE_MBEDTLS_BUILD_DIR}" EXCLUDE_FROM_ALL)

    if(NOT TARGET MbedTLS::MbedCrypto)
        if(TARGET tfpsacrypto)
            add_library(MbedTLS::MbedCrypto ALIAS tfpsacrypto)
        elseif(TARGET mbedcrypto)
            add_library(MbedTLS::MbedCrypto ALIAS mbedcrypto)
        endif()
    endif()

     # Object files merged into mbootcore via MonolithicArchive.cmake

     message(STATUS "")
    message(STATUS "Crypto support: ENABLED (Mbed TLS ${MBOOTCORE_MBEDTLS_VERSION})")
else()
    message(STATUS "Crypto support: DISABLED (MBOOTCORE_ENABLE_CRYPTO=OFF)")
endif()
