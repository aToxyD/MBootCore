# Hardware Validation Tests
# These tests require physical hardware in BootROM/EDL mode.
# They auto-skip when no compatible device is detected.

function(add_hardware_test TEST_NAME TEST_FILE)
    add_executable(${TEST_NAME} ${TEST_FILE})
    target_include_directories(${TEST_NAME}
        PRIVATE
            ${CMAKE_SOURCE_DIR}/lib/include
            ${CMAKE_CURRENT_SOURCE_DIR}
            ${CMAKE_SOURCE_DIR}/tests/mocks
    )
    target_link_libraries(${TEST_NAME}
        PRIVATE
            mbootcore
            Catch2::Catch2WithMain
    )
    add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
    set_tests_properties(${TEST_NAME} PROPERTIES LABELS "Hardware")
    # Catch2 v3 returns exit code 4 when all test cases are skipped.
    # CMake 3.24+ can interpret this as "skipped" via SKIP_RETURN_CODE.
    if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.24)
        set_property(TEST ${TEST_NAME} APPEND PROPERTY SKIP_RETURN_CODE 4)
    endif()
endfunction()

add_hardware_test(qualcomm_hardware_test qualcomm_hardware_test.cpp)
add_hardware_test(mediatek_hardware_test mediatek_hardware_test.cpp)
add_hardware_test(unisoc_hardware_test unisoc_hardware_test.cpp)
