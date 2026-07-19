add_library(mboot-doc-generator STATIC
    DocGenerator.cpp
)

target_link_libraries(mboot-doc-generator PUBLIC mbootcore)
target_include_directories(mboot-doc-generator PUBLIC
    ${CMAKE_SOURCE_DIR}/lib/include
    ${CMAKE_SOURCE_DIR}/sdk/include
)
mbootcore_apply_warnings(mboot-doc-generator)

add_library(mboot-plugin-wizard STATIC
    PluginWizard.cpp
)

target_link_libraries(mboot-plugin-wizard PUBLIC mbootcore)
target_include_directories(mboot-plugin-wizard PUBLIC
    ${CMAKE_SOURCE_DIR}/lib/include
    ${CMAKE_SOURCE_DIR}/sdk/include
)
mbootcore_apply_warnings(mboot-plugin-wizard)

install(TARGETS mboot-doc-generator mboot-plugin-wizard
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
