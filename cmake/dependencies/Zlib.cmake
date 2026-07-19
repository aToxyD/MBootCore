# ==========================================================================
# dependencies/Zlib.cmake
# --------------------------------------------------------------------------
# Compression library.  Always required.
# ==========================================================================

message(STATUS "")

set(_ZLIBSHIM_SHARED_LIBS ${BUILD_SHARED_LIBS})
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(ZLIB_BUILD_TESTING OFF CACHE BOOL "" FORCE)
set(ZLIB_BUILD_SHARED OFF CACHE BOOL "" FORCE)
set(ZLIB_BUILD_STATIC ON CACHE BOOL "" FORCE)
set(ZLIB_INSTALL OFF CACHE BOOL "" FORCE)

mbootcore_require_dependency(NAME zlib)

set(BUILD_SHARED_LIBS ${_ZLIBSHIM_SHARED_LIBS} CACHE BOOL "" FORCE)

if(NOT TARGET ZLIB::ZLIB)
    add_library(ZLIB::ZLIB INTERFACE IMPORTED)
    target_link_libraries(ZLIB::ZLIB INTERFACE zlibstatic)
    target_include_directories(ZLIB::ZLIB INTERFACE
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/deps/zlib/source>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/_deps_build/zlib>
    )
endif()
