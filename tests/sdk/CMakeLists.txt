function(add_sdk_test TEST_NAME TEST_FILE)
    add_executable(${TEST_NAME} ${TEST_FILE})
    target_include_directories(${TEST_NAME}
        PRIVATE
            ${CMAKE_SOURCE_DIR}/sdk/include
            ${CMAKE_SOURCE_DIR}/lib/include
            ${CMAKE_SOURCE_DIR}/tools
            ${CMAKE_CURRENT_SOURCE_DIR}/../mocks
    )
    target_compile_definitions(${TEST_NAME} PRIVATE
        MBOOTCORE_SOURCE_DIR="${CMAKE_SOURCE_DIR}"
    )
    target_link_libraries(${TEST_NAME}
        PRIVATE
            mbootcore
            Catch2::Catch2WithMain
    )
    mbootcore_apply_warnings(${TEST_NAME})
    target_compile_options(${TEST_NAME} PRIVATE
        $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-Wno-unused-result
        -Wno-unused-function
        -Wno-unused-parameter
        -Wno-unused-variable
        -Wno-missing-field-initializers>
    )
    add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
endfunction()

add_sdk_test(sdk_vendorsdk_test VendorSDKTest.cpp)
add_sdk_test(sdk_plugin_compatibility_test PluginCompatibilityTest.cpp)
add_sdk_test(sdk_manifest_validation_test ManifestValidationTest.cpp)
add_sdk_test(sdk_dependency_graph_test DependencyGraphTest.cpp)
add_sdk_test(sdk_registration_test RegistrationTest.cpp)
add_sdk_test(sdk_template_generation_test TemplateGenerationTest.cpp)
add_sdk_test(sdk_version_compatibility_test VersionCompatibilityTest.cpp)
add_sdk_test(sdk_doc_generator_test DocumentationGeneratorTest.cpp)
add_sdk_test(sdk_info_test SDKInfoTest.cpp)
add_sdk_test(sdk_doctor_test SDKDoctorTest.cpp)
add_sdk_test(sdk_env_checker_test EnvironmentCheckerTest.cpp)
add_sdk_test(sdk_version_test VersionTest.cpp)
add_sdk_test(sdk_validator_test SDKValidatorTest.cpp)
add_sdk_test(sdk_api_compatibility_test APICompatibilityTest.cpp)

# Safety-net: these tests require tool libraries from tools/.
# The root CMakeLists.txt enforces BUILD_TOOLS=ON when BUILD_TESTS=ON,
# but if(TARGET ...) guards prevent silent breakage from future refactors.
if(TARGET mboot-doc-generator)
    add_sdk_test(sdk_doc_generator_tool_test DocGeneratorToolTest.cpp)
    target_link_libraries(sdk_doc_generator_tool_test PRIVATE mboot-doc-generator)
endif()

if(TARGET mboot-plugin-wizard)
    add_sdk_test(sdk_plugin_wizard_test PluginWizardTest.cpp)
    target_link_libraries(sdk_plugin_wizard_test PRIVATE mboot-plugin-wizard)
endif()
