set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS
    Core Widgets Gui
    Network Concurrent
)

set(STUDIO_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)

file(GLOB_RECURSE STUDIO_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
)

file(GLOB_RECURSE STUDIO_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp
)

# Static library with all shared UI code
add_library(mbootstudio STATIC ${STUDIO_SOURCES} ${STUDIO_HEADERS})
target_include_directories(mbootstudio PUBLIC ${STUDIO_INCLUDE_DIR})
target_link_libraries(mbootstudio PUBLIC
    mbootcore
    Qt6::Core
    Qt6::Widgets
    Qt6::Gui
    Qt6::Network
    Qt6::Concurrent
)
mbootcore_apply_warnings(mbootstudio)

# Main executable
qt_add_executable(mboot-studio
    src/main.cpp
)
target_link_libraries(mboot-studio PRIVATE mbootstudio)
mbootcore_apply_warnings(mboot-studio)
set_target_properties(mboot-studio PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
    OUTPUT_NAME "mboot-studio"
)

# Tests
option(BUILD_STUDIO_TESTS "Build mboot-studio tests" ON)
if(BUILD_STUDIO_TESTS)
    find_package(Qt6 REQUIRED COMPONENTS Test)
    add_subdirectory(tests)
endif()

install(TARGETS mboot-studio
    BUNDLE DESTINATION ${CMAKE_INSTALL_BINDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
install(TARGETS mbootstudio
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
