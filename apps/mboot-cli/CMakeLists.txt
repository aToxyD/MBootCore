set(CLI_SOURCES
    main.cpp
    CliParser.cpp
    CliFormatter.cpp
    CliProgress.cpp
    CliCommands.cpp
    CliApplication.cpp
)

set(CLI_HEADERS
    CliParser.hpp
    CliFormatter.hpp
    CliProgress.hpp
    CliCommands.hpp
    CliApplication.hpp
)

add_executable(mboot-cli ${CLI_SOURCES} ${CLI_HEADERS})

target_include_directories(mboot-cli
    PRIVATE
        ${CMAKE_SOURCE_DIR}/lib/include
)

target_link_libraries(mboot-cli
    PRIVATE
        mbootcore
)

mbootcore_static_winpthread(mboot-cli)
mbootcore_apply_warnings(mboot-cli)

install(TARGETS mboot-cli
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
