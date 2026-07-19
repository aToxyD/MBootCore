include(${CMAKE_SOURCE_DIR}/cmake/dependencies/Catch2.cmake)

function(add_mbootcore_test TEST_NAME TEST_FILE)
    add_executable(${TEST_NAME} ${TEST_FILE})
    target_include_directories(${TEST_NAME}
        PRIVATE
            ${CMAKE_SOURCE_DIR}/lib/include
            ${CMAKE_SOURCE_DIR}/lib/src
            ${CMAKE_CURRENT_SOURCE_DIR}/mocks
            ${CMAKE_SOURCE_DIR}/tests/mocks
            ${CMAKE_SOURCE_DIR}/tests/virtual
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
        -Wno-unused-variable>
    )
    add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
endfunction()

function(add_mbootcore_test_with_src TEST_NAME TEST_FILE)
    add_executable(${TEST_NAME} ${TEST_FILE} ${ARGN})
    target_include_directories(${TEST_NAME}
        PRIVATE
            ${CMAKE_SOURCE_DIR}/lib/include
            ${CMAKE_SOURCE_DIR}/lib/src
            ${CMAKE_CURRENT_SOURCE_DIR}/mocks
            ${CMAKE_SOURCE_DIR}/tests/mocks
            ${CMAKE_SOURCE_DIR}/tests/virtual
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
        -Wno-unused-variable>
    )
    add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
endfunction()

# Core protocol tests
add_mbootcore_test(sahara_serializer_test core/protocols/sahara/SaharaPacketSerializerTest.cpp)
add_mbootcore_test(sahara_parser_test core/protocols/sahara/SaharaPacketParserTest.cpp)
add_mbootcore_test(state_machine_test core/state/StateMachineTest.cpp)
add_mbootcore_test(sahara_packets_test core/protocols/sahara/SaharaPacketsTest.cpp)
add_mbootcore_test(sahara_state_machine_test core/protocols/sahara/SaharaStateMachineTest.cpp)
add_mbootcore_test(sahara_protocol_test core/protocols/sahara/SaharaProtocolTest.cpp)
add_mbootcore_test(sahara_golden_vectors_test core/protocols/sahara/SaharaGoldenVectorsTest.cpp)
add_mbootcore_test(sahara_golden_vector_framework_test core/protocols/sahara/GoldenVectorFrameworkTest.cpp)
add_mbootcore_test(sahara_fuzz_test core/protocols/sahara/SaharaFuzzTest.cpp)
add_mbootcore_test(sahara_virtual_test virtual/SaharaVirtualTest.cpp)
add_mbootcore_test(firehose_golden_vectors_test core/protocols/firehose/FirehoseGoldenVectorsTest.cpp)
add_mbootcore_test(firehose_fuzz_test core/protocols/firehose/FirehoseFuzzTest.cpp)
add_mbootcore_test(firehose_virtual_test virtual/FirehoseVirtualTest.cpp)

add_mbootcore_test_with_src(generic_flash_test
    generic/GenericFlashTest.cpp
    virtual/VirtualFlashDevice.cpp
)

add_mbootcore_test(loader_framework_test
    loader/LoaderFrameworkTest.cpp
)

add_mbootcore_test(elf_engine_test
    elf/ElfEngineTest.cpp
)

add_mbootcore_test(firehose_stress_test
    firehose/FirehoseStressTest.cpp
)

add_mbootcore_test(firehose_fault_test
    firehose/FirehoseFaultInjectionTest.cpp
)

add_mbootcore_test(boot_pipeline_test
    pipeline/BootPipelineTest.cpp
)

add_mbootcore_test(gpt_models_test
    gpt/GPTModelsTest.cpp
)

add_mbootcore_test(gpt_parser_test
        gpt/GPTParserTest.cpp
    )
target_include_directories(gpt_parser_test PRIVATE
    ${CMAKE_SOURCE_DIR}/deps/zlib/source
    ${CMAKE_BINARY_DIR}/_deps_build/zlib
)

add_mbootcore_test(gpt_writer_test
    gpt/GPTWriterTest.cpp
)
target_include_directories(gpt_writer_test PRIVATE
    ${CMAKE_SOURCE_DIR}/deps/zlib/source
    ${CMAKE_BINARY_DIR}/_deps_build/zlib
)

add_mbootcore_test(gpt_manager_test
    gpt/PartitionManagerTest.cpp
)
target_include_directories(gpt_manager_test PRIVATE
    ${CMAKE_SOURCE_DIR}/deps/zlib/source
    ${CMAKE_BINARY_DIR}/_deps_build/zlib
)

add_mbootcore_test(discovery_test
    discovery/DiscoveryTest.cpp
)

add_mbootcore_test(session_test
    session/SessionTest.cpp
)

add_mbootcore_test(concurrency_test
    concurrency/ConcurrencyTest.cpp
)

add_mbootcore_test(plugin_test
    plugin/PluginTest.cpp
)

add_mbootcore_test(job_engine_test
    job/JobEngineTest.cpp
)

add_mbootcore_test(firmware_package_test
    firmware/FirmwarePackageTest.cpp
)

add_mbootcore_test(workflow_engine_test
    workflow/WorkflowEngineTest.cpp
)

add_mbootcore_test(vendor_framework_test
    vendor/VendorFrameworkTest.cpp
)

add_mbootcore_test(runtime_test
    runtime/RuntimeTest.cpp
)
set_tests_properties(runtime_test PROPERTIES LABELS "Environment")

# CLI tests
set(CLI_LIB_SOURCES
    ${CMAKE_SOURCE_DIR}/apps/mboot-cli/CliParser.cpp
    ${CMAKE_SOURCE_DIR}/apps/mboot-cli/CliFormatter.cpp
    ${CMAKE_SOURCE_DIR}/apps/mboot-cli/CliProgress.cpp
    ${CMAKE_SOURCE_DIR}/apps/mboot-cli/CliCommands.cpp
    ${CMAKE_SOURCE_DIR}/apps/mboot-cli/CliApplication.cpp
)
set(CLI_TEST_INCLUDES
    ${CMAKE_SOURCE_DIR}/apps/mboot-cli
    ${CMAKE_CURRENT_SOURCE_DIR}/virtual
)

function(add_cli_test TEST_NAME TEST_FILE)
    add_mbootcore_test_with_src(${TEST_NAME} ${TEST_FILE} ${CLI_LIB_SOURCES})
    target_include_directories(${TEST_NAME}
        PRIVATE ${CLI_TEST_INCLUDES}
    )
endfunction()

add_cli_test(cli_parser_test cli/CliParserTest.cpp)
add_cli_test(cli_formatter_test cli/CliFormatterTest.cpp)
add_cli_test(cli_command_test cli/CliCommandTest.cpp)
add_cli_test(cli_integration_test cli/CliIntegrationTest.cpp)
add_cli_test(cli_script_test cli/CliScriptTest.cpp)
add_cli_test(cli_virtual_test cli/VirtualRuntimeCliTest.cpp)

add_mbootcore_test(diagnostics_test
    diagnostics/DiagnosticsTest.cpp
)

add_mbootcore_test(telemetry_test
    telemetry/TelemetryTest.cpp
)

add_mbootcore_test(logging_test
    logging/LoggingTest.cpp
)

add_mbootcore_test_with_src(dsp_test
    dsp/DSPTest.cpp
)

add_mbootcore_test(profiler_test
    profiler/ProfilerTest.cpp
)

add_mbootcore_test(memory_tracker_test
    memory/MemoryTrackerTest.cpp
)

add_mbootcore_test(stress_test
    stress/StressTest.cpp
)

add_mbootcore_test(fault_test
    faults/FaultTest.cpp
)

add_mbootcore_test(recovery_test
    recovery/RecoveryTest.cpp
)

add_mbootcore_test(security_test
    security/SecurityTest.cpp
)

add_mbootcore_test(crypto_fallback_test
    security/CryptoFallbackTest.cpp
)

add_mbootcore_test(mbedtls_crypto_test
    security/MbedTLSCryptoTest.cpp
)
target_include_directories(mbedtls_crypto_test PRIVATE
    ${CMAKE_SOURCE_DIR}/deps/mbedtls/source/include
    ${CMAKE_SOURCE_DIR}/deps/mbedtls/source/tf-psa-crypto/include
    ${CMAKE_SOURCE_DIR}/deps/mbedtls/source/tf-psa-crypto/drivers/builtin/include
)

add_mbootcore_test(config_test
    config/ConfigTest.cpp
)

add_mbootcore_test(benchmark_test
    benchmark/BenchmarkTest.cpp
)

add_executable(compatibility_test
    compatibility/CompatibilityTest.cpp
)
target_include_directories(compatibility_test
    PRIVATE
        ${CMAKE_SOURCE_DIR}/lib/include
        ${CMAKE_SOURCE_DIR}/sdk/include
)
target_link_libraries(compatibility_test
    PRIVATE
        mbootcore
        Catch2::Catch2WithMain
)
target_compile_options(compatibility_test PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-Wno-unused-result
    -Wno-unused-function
    -Wno-unused-parameter
    -Wno-unused-variable>
)
mbootcore_apply_warnings(compatibility_test)
add_test(NAME compatibility_test COMMAND compatibility_test)

add_mbootcore_test(fuzz_test
    fuzz/FuzzTest.cpp
)

add_mbootcore_test(longrunning_test
    longrunning/LongRunningTest.cpp
)

add_mbootcore_test(dynamic_library_test
    platform/DynamicLibraryTest.cpp
)

add_mbootcore_test(protocol_semantic_design_test protocol/ProtocolSemanticDesignTest.cpp)
add_mbootcore_test(packet_serialization_test protocol/PacketSerializationTest.cpp)
if(MBOOTCORE_ENABLE_VENDOR_SCAFFOLDS)
    add_mbootcore_test(unisoc_protocol_test protocol/UNISOCProtocolTest.cpp)
    add_mbootcore_test(extensibility_proof_test protocol/ExtensibilityProofTest.cpp)
endif()
add_mbootcore_test(types_test domain/TypesTest.cpp)

# Expected backend names — mapped by CMake, not by #ifdef in tests
set(MBOOTCORE_EXPECTED_SERIAL_BACKEND "PosixTermios")
set(MBOOTCORE_EXPECTED_TCP_BACKEND "PosixTcpSocket")
set(MBOOTCORE_EXPECTED_UDP_BACKEND "PosixUdpSocket")
if(WIN32)
    set(MBOOTCORE_EXPECTED_SERIAL_BACKEND "Win32Serial")
    set(MBOOTCORE_EXPECTED_TCP_BACKEND "WinSockTcp")
    set(MBOOTCORE_EXPECTED_UDP_BACKEND "WinSockUdp")
endif()

add_mbootcore_test(backend_selection_test
    transport/BackendSelectionTest.cpp
)
set_tests_properties(backend_selection_test PROPERTIES LABELS "Environment")
target_compile_definitions(backend_selection_test PRIVATE
    MBOOTCORE_EXPECTED_SERIAL_BACKEND="${MBOOTCORE_EXPECTED_SERIAL_BACKEND}"
    MBOOTCORE_EXPECTED_TCP_BACKEND="${MBOOTCORE_EXPECTED_TCP_BACKEND}"
    MBOOTCORE_EXPECTED_UDP_BACKEND="${MBOOTCORE_EXPECTED_UDP_BACKEND}"
)

add_mbootcore_test(make_usb_backend_test
    transport/usb/MakeUsbBackendTest.cpp
)

add_mbootcore_test(usb_capability_test
    transport/usb/UsbCapabilityTest.cpp
)
if(MBOOTCORE_HAVE_USB OR WIN32)
    target_compile_definitions(usb_capability_test PRIVATE MBOOTCORE_TEST_USB_ENABLED=1)
    if(MBOOTCORE_HAVE_USB AND NOT WIN32)
        target_compile_definitions(usb_capability_test PRIVATE MBOOTCORE_TEST_USB_IS_LIBUSB=1)
    elseif(WIN32)
        target_compile_definitions(usb_capability_test PRIVATE MBOOTCORE_TEST_USB_IS_WINUSB=1)
    endif()
endif()

add_mbootcore_test(make_serial_backend_test
    transport/serial/MakeSerialBackendTest.cpp
)
target_compile_definitions(make_serial_backend_test PRIVATE
    MBOOTCORE_EXPECTED_SERIAL_BACKEND="${MBOOTCORE_EXPECTED_SERIAL_BACKEND}"
)

add_mbootcore_test(make_tcp_backend_test
    transport/network/MakeTcpBackendTest.cpp
)
target_compile_definitions(make_tcp_backend_test PRIVATE
    MBOOTCORE_EXPECTED_TCP_BACKEND="${MBOOTCORE_EXPECTED_TCP_BACKEND}"
)

add_mbootcore_test(make_udp_backend_test
    transport/network/MakeUdpBackendTest.cpp
)
target_compile_definitions(make_udp_backend_test PRIVATE
    MBOOTCORE_EXPECTED_UDP_BACKEND="${MBOOTCORE_EXPECTED_UDP_BACKEND}"
)

# SDK tests depend on tool libraries (mboot-doc-generator, mboot-plugin-wizard).
# Root CMakeLists.txt enforces BUILD_TOOLS=ON when BUILD_TESTS=ON.
add_subdirectory(sdk)

add_mbootcore_test(json_regression_test
    json/JsonRegressionTest.cpp
)
target_include_directories(json_regression_test PRIVATE
    ${CMAKE_SOURCE_DIR}/deps/nlohmann_json/source/include
)

add_mbootcore_test(network_address_test
    network/NetworkAddressTest.cpp
)

add_mbootcore_test(transport_test
    transport/TransportTest.cpp
)
set_tests_properties(transport_test PROPERTIES LABELS "Environment")

add_mbootcore_test(backend_mock_test
    transport/BackendMockTest.cpp
)

add_mbootcore_test(serial_enumerator_test
    transport/SerialEnumeratorTest.cpp
)

# Integration tests
function(add_integration_test TEST_NAME TEST_FILE)
    add_mbootcore_test(${TEST_NAME} ${TEST_FILE})
    set_tests_properties(${TEST_NAME} PROPERTIES LABELS "Integration")
endfunction()

add_integration_test(tcp_integration_test
    transport/TcpIntegrationTest.cpp
)

add_integration_test(udp_integration_test
    transport/UdpIntegrationTest.cpp
)

add_integration_test(serial_integration_test
    transport/SerialIntegrationTest.cpp
)

# Transport Benchmarks
function(add_benchmark_test TEST_NAME TEST_FILE)
    add_mbootcore_test(${TEST_NAME} ${TEST_FILE})
    set_tests_properties(${TEST_NAME} PROPERTIES LABELS "Benchmark")
endfunction()

add_benchmark_test(transport_benchmark
    transport/TransportBenchmark.cpp
)

# Dynamic plugin test — builds a test .so and loads it via PluginManager::loadFromFile
add_library(TestDynamicPlugin MODULE
    plugin/test_dynamic_plugin/TestDynamicPlugin.cpp
)
target_link_libraries(TestDynamicPlugin PRIVATE mbootcore)
target_compile_definitions(TestDynamicPlugin PRIVATE MBOOTCORE_PLUGIN_BUILD)
target_compile_features(TestDynamicPlugin PRIVATE cxx_std_17)
target_include_directories(TestDynamicPlugin PRIVATE
    ${CMAKE_SOURCE_DIR}/lib/include
)
target_compile_options(TestDynamicPlugin PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-Wno-unused-result
    -Wno-unused-function
    -Wno-unused-parameter
    -Wno-unused-variable>
)
mbootcore_apply_warnings(TestDynamicPlugin)
if(WIN32)
    set_target_properties(TestDynamicPlugin PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/plugins"
        PREFIX ""
    )
elseif(APPLE)
    set_target_properties(TestDynamicPlugin PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/plugins"
        PREFIX ""
        SUFFIX ".dylib"
    )
else()
    set_target_properties(TestDynamicPlugin PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/plugins"
        PREFIX ""
    )
endif()

set(DYNAMIC_PLUGIN_DIR "${CMAKE_CURRENT_BINARY_DIR}/plugins")
add_executable(dynamic_plugin_loading_test
    plugin/DynamicPluginLoadingTest.cpp
)
target_include_directories(dynamic_plugin_loading_test
    PRIVATE
        ${CMAKE_SOURCE_DIR}/lib/include
        ${CMAKE_SOURCE_DIR}/lib/src
)
target_compile_definitions(dynamic_plugin_loading_test
    PRIVATE
        MBOOTCORE_TEST_PLUGIN_DIR="${DYNAMIC_PLUGIN_DIR}"
)
target_link_libraries(dynamic_plugin_loading_test
    PRIVATE
        mbootcore
        Catch2::Catch2WithMain
)
mbootcore_apply_warnings(dynamic_plugin_loading_test)
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_link_options(dynamic_plugin_loading_test PRIVATE -rdynamic)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    target_link_options(dynamic_plugin_loading_test PRIVATE -rdynamic)
endif()
add_dependencies(dynamic_plugin_loading_test TestDynamicPlugin)
add_test(NAME dynamic_plugin_loading_test COMMAND dynamic_plugin_loading_test)
set_tests_properties(dynamic_plugin_loading_test PROPERTIES LABELS "Environment")


# Vendor maturity and capability tests
add_mbootcore_test(vendor_maturity_test
    vendor/VendorMaturityTest.cpp
)

# ---------------------------------------------------------------------------
# Security & Robustness Testing (gated behind MBOOTCORE_BUILD_SECURITY_TESTS)
# ---------------------------------------------------------------------------
if(MBOOTCORE_BUILD_SECURITY_TESTS)
    add_mbootcore_test(safe_parser_test
        security/SafeParserTest.cpp
    )
    set_tests_properties(safe_parser_test PROPERTIES LABELS "Security")
    mbootcore_apply_sanitizers(safe_parser_test)

    add_mbootcore_test(parser_robustness_test
        security/ParserRobustnessTest.cpp
    )
    set_tests_properties(parser_robustness_test PROPERTIES LABELS "Security")

    # Sanitizer flags for security tests
    mbootcore_apply_sanitizers(parser_robustness_test)
endif()

# ---------------------------------------------------------------------------
# Build-Option Dependency Validation
# ---------------------------------------------------------------------------
# Verifies that the four BUILD_TESTS × BUILD_TOOLS combinations behave
# correctly at configure time.  Run via:  ctest -R build_options_validation
# ---------------------------------------------------------------------------
add_test(NAME build_options_validation
    COMMAND ${CMAKE_COMMAND}
        -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/BuildOptionsValidation.cmake
)
set_tests_properties(build_options_validation PROPERTIES
    LABELS "BuildValidation"
    TIMEOUT 1800
)

# ---------------------------------------------------------------------------
# Export Validation Gate — Monolithic Architecture Regression
# ---------------------------------------------------------------------------
# 3 hermetic regression gates that prevent dependency leaks and verify
# real-world package consumption.  Requires a prior build + install.
# Run via:  ctest -R export_validation
# ---------------------------------------------------------------------------
add_test(NAME export_validation
    COMMAND ${CMAKE_COMMAND}
        -DBUILD_DIR=${CMAKE_BINARY_DIR}
        -DCMAKE_INSTALL_DOCDIR=${CMAKE_INSTALL_DOCDIR}
        -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/ExportValidation.cmake
)
set_tests_properties(export_validation PROPERTIES LABELS "BuildValidation")

# ---------------------------------------------------------------------------
# Fuzz Harnesses (gated behind MBOOTCORE_BUILD_FUZZERS, requires libFuzzer)
# ---------------------------------------------------------------------------
if(MBOOTCORE_BUILD_FUZZERS)
    function(add_fuzz_target FUZZ_NAME FUZZ_FILE)
        add_executable(${FUZZ_NAME} ${FUZZ_FILE})
        target_include_directories(${FUZZ_NAME}
            PRIVATE
                ${CMAKE_SOURCE_DIR}/lib/include
                ${CMAKE_SOURCE_DIR}/lib/src
        )
        target_link_libraries(${FUZZ_NAME}
            PRIVATE
                mbootcore
                -fsanitize=fuzzer,address,undefined
        )
        target_compile_options(${FUZZ_NAME} PRIVATE
            -fsanitize=fuzzer,address,undefined
            -fno-omit-frame-pointer
        )
        target_compile_definitions(${FUZZ_NAME} PRIVATE MBOOTCORE_HAS_LIBFUZZER=1)
    endfunction()

    add_fuzz_target(fuzz_sahara_parser fuzz/FuzzSaharaParser.cpp)
    add_fuzz_target(fuzz_firehose_xml fuzz/FuzzFirehoseXml.cpp)
    add_fuzz_target(fuzz_elf_parser fuzz/FuzzElfParser.cpp)
endif()

# Hardware Validation Tests
add_subdirectory(hardware)
