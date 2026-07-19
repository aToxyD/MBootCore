# ==========================================================================
# QualityGates.cmake
# --------------------------------------------------------------------------
# Configure-time validation gates for MBootCore.
# Fails CMake configure if any quality violation is detected.
# ==========================================================================

# ---------------------------------------------------------------------------
# Gate 1: Forbidden deprecated headers
# ---------------------------------------------------------------------------
set(_QG_DEPRECATED_HEADERS
    "SerialBackend.hpp"
    "TcpBackend.hpp"
    "UdpBackend.hpp"
)

foreach(hdr ${_QG_DEPRECATED_HEADERS})
    file(GLOB_RECURSE _match
        "${CMAKE_SOURCE_DIR}/lib/include/mbootcore/**/${hdr}"
    )
    if(_match)
        message(SEND_ERROR
            "QualityGate FAILED: deprecated header '${hdr}' found in lib/include/\n"
            "  Remove or migrate consumers to the new naming.\n"
            "  Match: ${_match}"
        )
    endif()
endforeach()

# ---------------------------------------------------------------------------
# Gate 2: Duplicate ProtocolVersion definitions
# ---------------------------------------------------------------------------
file(GLOB_RECURSE _pv_files
    "${CMAKE_SOURCE_DIR}/lib/include/mbootcore/*.hpp"
)
foreach(f ${_pv_files})
    file(STRINGS "${f}" _pv_lines REGEX "struct ProtocolVersion")
    if(_pv_lines)
        get_filename_component(_basename "${f}" NAME)
        if(NOT _basename STREQUAL "ProtocolVersion.hpp" AND
           NOT _basename STREQUAL "Types.hpp" AND
           NOT _basename STREQUAL "SaharaProtocolTypes.hpp")
            message(SEND_ERROR
                "QualityGate FAILED: unexpected 'struct ProtocolVersion' in ${f}\n"
                "  ProtocolVersion is defined in domain/Types.hpp.\n"
                "  Vendor-specific versions should be prefixed (e.g. SaharaNegotiatedVersion)."
            )
        endif()
    endif()
endforeach()

# ---------------------------------------------------------------------------
# Gate 3: Forbidden TODO/FIXME in production sources (excluding tests/examples)
# ---------------------------------------------------------------------------
set(_QG_PRODUCTION_DIRS
    "${CMAKE_SOURCE_DIR}/lib"
    "${CMAKE_SOURCE_DIR}/sdk"
    "${CMAKE_SOURCE_DIR}/apps/mboot-cli"
    "${CMAKE_SOURCE_DIR}/tools"
    "${CMAKE_SOURCE_DIR}/cmake"
)

foreach(d ${_QG_PRODUCTION_DIRS})
    file(GLOB_RECURSE _src_files "${d}/*.cpp" "${d}/*.hpp" "${d}/*.h" "${d}/*.cmake" "${d}/CMakeLists.txt")
    foreach(f ${_src_files})
        get_filename_component(_qg_base "${f}" NAME)
        if(_qg_base STREQUAL "QualityGates.cmake")
            continue()
        endif()
        file(STRINGS "${f}" _todo_lines REGEX "(TODO|FIXME|HACK|XXX)[ :]")
        if(_todo_lines)
            message(SEND_ERROR
                "QualityGate FAILED: TODO/FIXME/HACK/XXX found in production source\n"
                "  File: ${f}\n"
                "  Violations:\n"
            )
            foreach(line ${_todo_lines})
                string(STRIP "${line}" _clean)
                message("    ${_clean}")
            endforeach()
        endif()
    endforeach()
endforeach()

# ---------------------------------------------------------------------------
# Gate 4: Accidental placeholder security implementations
# ---------------------------------------------------------------------------
set(_QG_SECURITY_PATTERNS
    "fake"
    "placeholder"
    "dummy"
    "stub"
    "TODO.*security"
    "FIXME.*security"
)

file(GLOB_RECURSE _sec_src
    "${CMAKE_SOURCE_DIR}/lib/src/security/*.cpp"
    "${CMAKE_SOURCE_DIR}/lib/include/mbootcore/security/*.hpp"
)
foreach(f ${_sec_src})
    file(STRINGS "${f}" _sec_lines)
    set(_line_num 0)
    foreach(line ${_sec_lines})
        math(EXPR _line_num "${_line_num} + 1")
        foreach(pattern ${_QG_SECURITY_PATTERNS})
            string(TOLOWER "${line}" _lc_line)
            string(TOLOWER "${pattern}" _lc_pattern)
            string(REGEX MATCH "${_lc_pattern}" _match "${_lc_line}")
            if(_match)
                string(REGEX MATCH "(static|const|auto|return|class|struct|\"[^\"]*${_lc_pattern}[^\"]*\")" _code_match "${_lc_line}")
                if(_code_match)
                    message(SEND_ERROR
                        "QualityGate FAILED: potential placeholder implementation in security code\n"
                        "  File: ${f}:${_line_num}\n"
                        "  Pattern: '${pattern}'\n"
                        "  Line: ${line}"
                    )
                endif()
            endif()
        endforeach()
    endforeach()
endforeach()

message(STATUS "QualityGates: passed — all configure-time validations successful")

# ---------------------------------------------------------------------------
# Gate 5: Crypto provider validation (only when MBOOTCORE_ENABLE_CRYPTO=ON)
# ---------------------------------------------------------------------------
if(MBOOTCORE_ENABLE_CRYPTO)
    # Check that MbedTLS provider files exist
    set(_QG_MBEDTLS_PROVIDER_FILES
        "${CMAKE_SOURCE_DIR}/lib/src/security/mbedtls/MbedTlsUtils.cpp"
        "${CMAKE_SOURCE_DIR}/lib/src/security/mbedtls/MbedTLSHashProvider.cpp"
        "${CMAKE_SOURCE_DIR}/lib/src/security/mbedtls/MbedTLSSignatureVerifier.cpp"
        "${CMAKE_SOURCE_DIR}/lib/src/security/mbedtls/MbedTLSIntegrityVerifier.cpp"
    )
    foreach(f ${_QG_MBEDTLS_PROVIDER_FILES})
        if(NOT EXISTS "${f}")
            message(SEND_ERROR
                "QualityGate FAILED: MBOOTCORE_ENABLE_CRYPTO=ON but MbedTLS "
                "provider file missing: ${f}\n"
                "  Either disable crypto (MBOOTCORE_ENABLE_CRYPTO=OFF) or "
                "implement the provider."
            )
        endif()
    endforeach()

    # Check that FirmwareValidator has the crypto-aware integrity method
    file(STRINGS "${CMAKE_SOURCE_DIR}/lib/src/firmware/FirmwareValidator.cpp"
        _FV_CRYPTO_LINES REGEX "verifyHashWithProvider")
    if(NOT _FV_CRYPTO_LINES)
        message(SEND_ERROR
            "QualityGate FAILED: MBOOTCORE_ENABLE_CRYPTO=ON but "
            "FirmwareValidator does not use cryptographic verification.\n"
            "  verifyHashWithProvider must be implemented in FirmwareValidator.cpp."
        )
    endif()

    message(STATUS "QualityGates (crypto): passed — MbedTLS provider validated")
endif()

# ---------------------------------------------------------------------------
# Gate 6: USB dependency model validation (only when MBOOTCORE_ENABLE_USB=ON)
# ---------------------------------------------------------------------------
if(MBOOTCORE_ENABLE_USB)
    if(MBOOTCORE_HAVE_USB)
        if(NOT TARGET LibUSB::LibUSB)
            message(SEND_ERROR
                "QualityGate FAILED: MBOOTCORE_ENABLE_USB=ON and USB was found, "
                "but the LibUSB::LibUSB imported target does not exist.\n"
                "  The target must be created in the USB dependency block."
            )
        endif()
        if(NOT MBOOTCORE_USB_SOURCE)
            message(SEND_ERROR
                "QualityGate FAILED: MBOOTCORE_ENABLE_USB=ON and USB was found, "
                "but MBOOTCORE_USB_SOURCE is not set.\n"
                "  USB backend source must be recorded for diagnostics."
            )
        endif()
        message(STATUS "QualityGates (usb): passed — LibUSB::LibUSB target exists, "
                       "source: ${MBOOTCORE_USB_SOURCE}")
    else()
        message(STATUS "QualityGates (usb): passed — USB disabled (libusb not found)")
    endif()
else()
    message(STATUS "QualityGates (usb): passed — USB disabled by MBOOTCORE_ENABLE_USB=OFF")
endif()

# ---------------------------------------------------------------------------
# Gate 7: Version consistency (VERSION file must match CMakeLists.txt project)
# ---------------------------------------------------------------------------
file(READ "${CMAKE_SOURCE_DIR}/VERSION" _QG_VERSION_FILE)
string(STRIP "${_QG_VERSION_FILE}" _QG_VERSION_FILE)

if(NOT _QG_VERSION_FILE STREQUAL "${PROJECT_VERSION}")
    message(SEND_ERROR
        "QualityGate FAILED: VERSION file and CMakeLists.txt project version differ\n"
        "  VERSION file:   ${_QG_VERSION_FILE}\n"
        "  CMakeLists.txt: ${PROJECT_VERSION}\n"
        "  Update VERSION or project() to be consistent."
    )
else()
    message(STATUS "QualityGates (version): passed — VERSION file matches project version ${PROJECT_VERSION}")
endif()

# ---------------------------------------------------------------------------
# Gate 8: Public header self-containment (critical headers must compile alone)
# ---------------------------------------------------------------------------
set(_QG_SELF_CONTAINMENT_HEADERS
    "${CMAKE_SOURCE_DIR}/lib/include/mbootcore/application/Session.hpp"
    "${CMAKE_SOURCE_DIR}/lib/include/mbootcore/runtime/Runtime.hpp"
    "${CMAKE_SOURCE_DIR}/lib/include/mbootcore/domain/Error.hpp"
    "${CMAKE_SOURCE_DIR}/lib/include/mbootcore/vendor/VendorTypes.hpp"
    "${CMAKE_SOURCE_DIR}/lib/include/mbootcore/transport/TransportTypes.hpp"
)

set(_QG_SELF_CONTAINMENT_FAILED FALSE)
foreach(hdr ${_QG_SELF_CONTAINMENT_HEADERS})
    get_filename_component(_hdr_name "${hdr}" NAME)
    file(WRITE "${CMAKE_BINARY_DIR}/_qg_selfcontain_${_hdr_name}.cpp"
        "#include \"${hdr}\"\nint main() { return 0; }\n"
    )
    try_compile(
        _qg_compile_result
        "${CMAKE_BINARY_DIR}/_qg_selfcontain"
        "${CMAKE_BINARY_DIR}/_qg_selfcontain_${_hdr_name}.cpp"
        CMAKE_FLAGS
            "-DINCLUDE_DIRECTORIES:STRING=${CMAKE_SOURCE_DIR}/lib/include"
        OUTPUT_VARIABLE _qg_compile_output
    )
    if(NOT _qg_compile_result)
        set(_QG_SELF_CONTAINMENT_FAILED TRUE)
        message(WARNING
            "QualityGate FAILED: public header '${_hdr_name}' is not self-contained\n"
            "  ${_qg_compile_output}"
        )
    endif()
endforeach()

if(_QG_SELF_CONTAINMENT_FAILED)
    message(SEND_ERROR
        "QualityGate FAILED: one or more public headers are not self-contained.\n"
        "  Each public header must compile standalone without implicit dependencies."
    )
else()
    message(STATUS "QualityGates (self-containment): passed — all 5 critical public headers compile standalone")
endif()

# ---------------------------------------------------------------------------
# Gate 9: Deprecated annotations in public API headers
# ---------------------------------------------------------------------------
set(_QG_DEPRECATION_PATTERNS
    "\\[\\[deprecated\\]\\]"
    "__attribute__\\(\\(deprecated\\)\\)"
    "__declspec\\(deprecated\\)"
)

set(_QG_DEPRECATION_FOUND FALSE)
file(GLOB_RECURSE _qg_api_headers
    "${CMAKE_SOURCE_DIR}/lib/include/mbootcore/*.hpp"
)
foreach(hdr ${_qg_api_headers})
    foreach(pat ${_QG_DEPRECATION_PATTERNS})
        file(STRINGS "${hdr}" _dep_lines REGEX "${pat}")
        if(_dep_lines)
            set(_QG_DEPRECATION_FOUND TRUE)
            foreach(line ${_dep_lines})
                string(STRIP "${line}" _dep_clean)
                message(WARNING
                    "QualityGate WARNING: deprecated annotation in public header\n"
                    "  File: ${hdr}\n"
                    "  Line: ${_dep_clean}"
                )
            endforeach()
        endif()
    endforeach()
endforeach()

if(_QG_DEPRECATION_FOUND)
    message(SEND_ERROR
        "QualityGate FAILED: deprecated annotations found in public API headers.\n"
        "  Remove deprecated markers before releasing. Use API_VERSION for compat."
    )
else()
    message(STATUS "QualityGates (abr): passed — no deprecated annotations in public API headers")
endif()

# ---------------------------------------------------------------------------
# Gate 11: Test coverage per source file
# ---------------------------------------------------------------------------
file(GLOB_RECURSE _qg_lib_sources RELATIVE "${CMAKE_SOURCE_DIR}"
    "lib/src/*.cpp"
)
file(GLOB_RECURSE _qg_test_sources RELATIVE "${CMAKE_SOURCE_DIR}"
    "tests/**/*.cpp"
)
# Remove test helper files and mocks from the count
list(FILTER _qg_test_sources EXCLUDE REGEX "tests/mocks/")
list(FILTER _qg_test_sources EXCLUDE REGEX "tests/virtual/VirtualFlashDevice\\.cpp$")

set(_qg_src_count 0)
set(_qg_test_count 0)
list(LENGTH _qg_lib_sources _qg_src_count)
list(LENGTH _qg_test_sources _qg_test_count)

if(_qg_src_count GREATER 0)
    math(EXPR _qg_min_tests "${_qg_src_count} / 3")
    if(_qg_test_count LESS _qg_min_tests)
        message(SEND_ERROR
            "QualityGate FAILED: insufficient test coverage for lib/src/ files\n"
            "  lib/src/ files: ${_qg_src_count}\n"
            "  test files:     ${_qg_test_count}\n"
            "  Minimum expected (src/3): ${_qg_min_tests}\n"
            "  Each source file should have at least one corresponding test."
        )
    else()
        message(STATUS "QualityGates (test-coverage): passed — ${_qg_test_count} test files cover ${_qg_src_count} source files (min: ${_qg_min_tests})")
    endif()
else()
    message(STATUS "QualityGates (test-coverage): skipped — no lib/src/ files found")
endif()
