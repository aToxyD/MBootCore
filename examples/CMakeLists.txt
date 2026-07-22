if(MBOOTCORE_BUILD_EXAMPLES)

    function(add_mbootcore_example name)
        add_executable(${name} ${name}/main.cpp)
        target_link_libraries(${name} PRIVATE mbootcore)
        target_compile_features(${name} PRIVATE cxx_std_17)
        mbootcore_apply_warnings(${name})
    endfunction()

    add_mbootcore_example(minimal)
    add_mbootcore_example(sahara)
    add_mbootcore_example(firehose)

    # Vendor SDK examples
    function(add_sdk_example name)
        add_executable(${name} sdk/${name}.cpp)
        target_link_libraries(${name} PRIVATE mbootcore)
        target_include_directories(${name} PRIVATE ${CMAKE_SOURCE_DIR}/sdk/include)
        target_compile_features(${name} PRIVATE cxx_std_17)
        mbootcore_apply_warnings(${name})
    endfunction()

    add_sdk_example(qualcomm_vendor_plugin)
    add_sdk_example(vendor_registration_example)
    add_sdk_example(dummy_vendor_plugin)
    add_sdk_example(dummy_protocol_plugin)
    add_sdk_example(dummy_workflow_plugin)
    add_sdk_example(dummy_package_plugin)
    add_sdk_example(dummy_discovery_plugin)

    # Developer Examples
    function(add_developer_example name)
        add_executable(example_${name} ${name}/main.cpp)
        target_link_libraries(example_${name} PRIVATE mbootcore)
        target_include_directories(example_${name} PRIVATE
            ${CMAKE_SOURCE_DIR}/lib/include
            ${CMAKE_SOURCE_DIR}/sdk/include
        )
        target_compile_features(example_${name} PRIVATE cxx_std_17)
        if(MBOOTCORE_BUILD_CLI)
            target_include_directories(example_${name} PRIVATE ${CMAKE_SOURCE_DIR}/apps/mboot-cli)
        endif()
        mbootcore_apply_warnings(example_${name})
    endfunction()

    add_developer_example(basic_connect)
    add_developer_example(device_discovery)
    add_developer_example(backup_partition)
    add_developer_example(plugin_creation)
    add_developer_example(custom_vendor)
    add_developer_example(workflow_execution)
    add_developer_example(job_pipeline)
    add_developer_example(flash_partition)
    add_developer_example(runtime_usage)
    add_developer_example(cli_embedding)

    # Dynamic plugin examples (.so/.dll)
    function(add_dynamic_plugin_example name)
        add_library(${name} MODULE
            dynamic/${name}/${name}.cpp
        )
        target_link_libraries(${name} PRIVATE mbootcore)
        # Plugin implementations must export their ABI symbols on Windows.
        # The plugin ABI header switches between dllexport/dllimport based on
        # MBOOTCORE_PLUGIN_BUILD.
        target_compile_definitions(${name} PRIVATE MBOOTCORE_PLUGIN_BUILD)
        target_compile_features(${name} PRIVATE cxx_std_17)
        mbootcore_apply_warnings(${name})
        if(WIN32)
            set_target_properties(${name} PROPERTIES
                LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/plugins"
                PREFIX ""
            )
        elseif(APPLE)
            set_target_properties(${name} PROPERTIES
                LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/plugins"
                PREFIX ""
                SUFFIX ".dylib"
            )
        else()
            set_target_properties(${name} PROPERTIES
                LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/plugins"
                PREFIX ""
            )
        endif()
    endfunction()

    add_dynamic_plugin_example(HelloPlugin)
    add_dynamic_plugin_example(ProtocolPlugin)
endif()
