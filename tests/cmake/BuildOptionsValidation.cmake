# ==========================================================================
# tests/cmake/BuildOptionsValidation.cmake
# --------------------------------------------------------------------------
# Validates the build-option dependency matrix for MBOOTCORE_BUILD_TESTS
# and MBOOTCORE_BUILD_TOOLS.
#
# Expected outcomes:
#   BUILD_TESTS=OFF  BUILD_TOOLS=OFF  → CONFIGURE_SUCCEEDED
#   BUILD_TESTS=OFF  BUILD_TOOLS=ON   → CONFIGURE_SUCCEEDED
#   BUILD_TESTS=ON   BUILD_TOOLS=ON   → CONFIGURE_SUCCEEDED
#   BUILD_TESTS=ON   BUILD_TOOLS=OFF  → CONFIGURE_FAILED (FATAL_ERROR)
#
# Run via:  cmake -P tests/cmake/BuildOptionsValidation.cmake
# ==========================================================================

cmake_minimum_required(VERSION 3.20)

set(SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/../..")
set(BASE_DIR "${CMAKE_CURRENT_LIST_DIR}/_build_validation")
file(REMOVE_RECURSE "${BASE_DIR}")

set(EXIT_CODE 0)

function(run_configure BUILD_TESTS BUILD_TOOLS EXPECT_FAIL TEST_LABEL)
    set(BUILD_DIR "${BASE_DIR}/${TEST_LABEL}")
    file(MAKE_DIRECTORY "${BUILD_DIR}")

    execute_process(
        COMMAND ${CMAKE_COMMAND}
            -S "${SOURCE_DIR}"
            -B "${BUILD_DIR}"
            -DCMAKE_BUILD_TYPE=Debug
            -DMBOOTCORE_BUILD_TESTS=${BUILD_TESTS}
            -DMBOOTCORE_BUILD_TOOLS=${BUILD_TOOLS}
            -DMBOOTCORE_BUILD_STUDIO=OFF
            -DMBOOTCORE_BUILD_CLI=OFF
            -DMBOOTCORE_BUILD_EXAMPLES=OFF
            -DMBOOTCORE_ENABLE_USB=OFF
        OUTPUT_VARIABLE STDOUT
        ERROR_VARIABLE  STDERR
        RESULT_VARIABLE CONFIG_RESULT
        TIMEOUT 450
    )

    set(LOG "${STDOUT}\n${STDERR}")
    file(WRITE "${BUILD_DIR}/cmake_output.log" "${LOG}")

    if(EXPECT_FAIL)
        if(CONFIG_RESULT EQUAL 0)
            message(STATUS "[FAIL] ${TEST_LABEL}: expected configure FAILURE but it succeeded")
            set(EXIT_CODE 1 PARENT_SCOPE)
        else()
            string(FIND "${LOG}" "MBOOTCORE_BUILD_TESTS=ON requires MBOOTCORE_BUILD_TOOLS=ON" _pos)
            if(_pos EQUAL -1)
                message(STATUS "[FAIL] ${TEST_LABEL}: configure failed but with wrong message")
                message(STATUS "  Output: ${LOG}")
                set(EXIT_CODE 1 PARENT_SCOPE)
            else()
                message(STATUS "[PASS] ${TEST_LABEL}: configure correctly failed with expected message")
            endif()
        endif()
    else()
        if(CONFIG_RESULT EQUAL 0)
            message(STATUS "[PASS] ${TEST_LABEL}: configure succeeded as expected")
        else()
            message(STATUS "[FAIL] ${TEST_LABEL}: expected configure SUCCESS but it failed")
            message(STATUS "  Output: ${LOG}")
            set(EXIT_CODE 1 PARENT_SCOPE)
        endif()
    endif()
endfunction()

message(STATUS "")
message(STATUS "=== Build-Option Validation Tests ===")

run_configure(OFF OFF FALSE "tests_off_tools_off")
run_configure(OFF ON  FALSE "tests_off_tools_on")
run_configure(ON  ON  FALSE "tests_on_tools_on")
run_configure(ON  OFF TRUE  "tests_on_tools_off")

file(REMOVE_RECURSE "${BASE_DIR}")

message(STATUS "")
if(EXIT_CODE EQUAL 0)
    message(STATUS "=== All build-option validation tests passed ===")
else()
    message(FATAL_ERROR "=== Build-option validation tests FAILED ===")
endif()
