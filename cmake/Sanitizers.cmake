# Sanitizers.cmake — Optional sanitizer support.
#
# Provides three CMake options:
#   MBOOTCORE_ENABLE_ASAN   — AddressSanitizer (heap/stack overflow, UAF, double-free)
#   MBOOTCORE_ENABLE_UBSAN  — UndefinedBehaviorSanitizer (integer UB, alignment, etc.)
#   MBOOTCORE_ENABLE_TSAN   — ThreadSanitizer (data races, deadlocks)
#
# Sanitizers are silently ignored for Release/RelWithDebInfo builds and
# unsupported compilers.  Never apply sanitizer flags to third-party /
# external targets.
#
# Mutual exclusion:
#   ASan + TSan  → FATAL_ERROR (never supported)
#   TSan + UBSan → WARNING (not officially tested, may vary)

option(MBOOTCORE_ENABLE_ASAN  "Enable AddressSanitizer (Debug builds only)"  OFF)
option(MBOOTCORE_ENABLE_UBSAN "Enable UndefinedBehaviorSanitizer (Debug builds only)" OFF)
option(MBOOTCORE_ENABLE_TSAN  "Enable ThreadSanitizer (Debug builds only)"  OFF)

# ---------------------------------------------------------------------------
# Build-type gate
# ---------------------------------------------------------------------------
if(CMAKE_BUILD_TYPE STREQUAL "Release" OR CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
    if(MBOOTCORE_ENABLE_ASAN)
        message(STATUS "MBootCore: ASan ignored in Release/RelWithDebInfo builds")
        set(MBOOTCORE_ENABLE_ASAN OFF CACHE BOOL "" FORCE)
    endif()
    if(MBOOTCORE_ENABLE_UBSAN)
        message(STATUS "MBootCore: UBSan ignored in Release/RelWithDebInfo builds")
        set(MBOOTCORE_ENABLE_UBSAN OFF CACHE BOOL "" FORCE)
    endif()
    if(MBOOTCORE_ENABLE_TSAN)
        message(STATUS "MBootCore: TSan ignored in Release/RelWithDebInfo builds")
        set(MBOOTCORE_ENABLE_TSAN OFF CACHE BOOL "" FORCE)
    endif()
endif()

# ---------------------------------------------------------------------------
# Mutual exclusion
# ---------------------------------------------------------------------------
if(MBOOTCORE_ENABLE_TSAN AND MBOOTCORE_ENABLE_ASAN)
    message(FATAL_ERROR
        "MBootCore: TSan and ASan are mutually exclusive. "
        "Enable only one at a time.")
endif()

if(MBOOTCORE_ENABLE_TSAN AND MBOOTCORE_ENABLE_UBSAN)
    message(WARNING
        "MBootCore: TSan + UBSan combination is not officially tested. "
        "Results may vary across compilers and versions.")
endif()

# ---------------------------------------------------------------------------
# Compiler support detection
#
# check_cxx_compiler_flag passes flags as COMPILE_DEFINITIONS (-D) on some
# CMake versions, which silently breaks sanitizer flag detection.  We use
# check_cxx_source_compiles with CMAKE_REQUIRED_FLAGS instead — this
# exercises the real compile + link path and is reliable across CMake versions.
# ---------------------------------------------------------------------------
include(CheckCXXSourceCompiles)

set(_MBOOTCORE_SANITIZER_FLAGS "")
set(_MBOOTCORE_SANITIZER_LINK_FLAGS "")

macro(_mbootcore_check_sanitizer FLAG RESULT_VAR)
    set(CMAKE_REQUIRED_FLAGS "${FLAG}")
    check_cxx_source_compiles("int main(){return 0;}" ${RESULT_VAR})
    unset(CMAKE_REQUIRED_FLAGS)
endmacro()

if(MBOOTCORE_ENABLE_ASAN)
    _mbootcore_check_sanitizer("-fsanitize=address" _mbootcore_asan_supported)
    if(_mbootcore_asan_supported)
        list(APPEND _MBOOTCORE_SANITIZER_FLAGS -fsanitize=address -fno-omit-frame-pointer)
        list(APPEND _MBOOTCORE_SANITIZER_LINK_FLAGS -fsanitize=address)
        message(STATUS "MBootCore: AddressSanitizer ENABLED")
    else()
        message(WARNING "MBootCore: ASan requested but compiler does not support -fsanitize=address")
        set(MBOOTCORE_ENABLE_ASAN OFF CACHE BOOL "" FORCE)
    endif()
endif()

if(MBOOTCORE_ENABLE_UBSAN)
    _mbootcore_check_sanitizer("-fsanitize=undefined" _mbootcore_ubsan_supported)
    if(_mbootcore_ubsan_supported)
        list(APPEND _MBOOTCORE_SANITIZER_FLAGS -fsanitize=undefined -fno-omit-frame-pointer)
        list(APPEND _MBOOTCORE_SANITIZER_LINK_FLAGS -fsanitize=undefined)
        message(STATUS "MBootCore: UndefinedBehaviorSanitizer ENABLED")
    else()
        message(WARNING "MBootCore: UBSan requested but compiler does not support -fsanitize=undefined")
        set(MBOOTCORE_ENABLE_UBSAN OFF CACHE BOOL "" FORCE)
    endif()
endif()

if(MBOOTCORE_ENABLE_TSAN)
    _mbootcore_check_sanitizer("-fsanitize=thread" _mbootcore_tsan_supported)
    if(_mbootcore_tsan_supported)
        list(APPEND _MBOOTCORE_SANITIZER_FLAGS -fsanitize=thread -fno-omit-frame-pointer)
        list(APPEND _MBOOTCORE_SANITIZER_LINK_FLAGS -fsanitize=thread)
        message(STATUS "MBootCore: ThreadSanitizer ENABLED")
    else()
        message(WARNING "MBootCore: TSan requested but compiler does not support -fsanitize=thread")
        set(MBOOTCORE_ENABLE_TSAN OFF CACHE BOOL "" FORCE)
    endif()
endif()

# ---------------------------------------------------------------------------
# mbootcore_apply_sanitizers(<target>)
#
# Apply sanitizer flags to a single target.  Call once per target, after
# creation.  Only applied when at least one MBOOTCORE_ENABLE_* option is ON.
#
# OBJECT libraries receive compile flags only — link flags are a no-op on
# OBJECT libraries and must be applied to the consuming STATIC/SHARED target.
#
# STATIC libraries use PUBLIC link scope so that every consuming executable
# automatically links the sanitizer runtime.  Without this, executables
# linking a sanitizer-instrumented static archive would fail with undefined
# reference errors (e.g., __tsan_*).
# ---------------------------------------------------------------------------
function(mbootcore_apply_sanitizers TARGET)
    if(NOT TARGET ${TARGET})
        message(FATAL_ERROR "mbootcore_apply_sanitizers: unknown target '${TARGET}'")
    endif()

    if(_MBOOTCORE_SANITIZER_FLAGS)
        target_compile_options(${TARGET} PRIVATE ${_MBOOTCORE_SANITIZER_FLAGS})
    endif()

    get_target_property(_type ${TARGET} TYPE)
    if(NOT _type STREQUAL "OBJECT_LIBRARY")
        if(_MBOOTCORE_SANITIZER_LINK_FLAGS)
            if(_type STREQUAL "STATIC_LIBRARY")
                # STATIC libraries do not perform the final link step, but
                # their object files reference sanitizer symbols. Consumers
                # must link the sanitizer runtime.  PUBLIC propagates the
                # required linker options to all consuming executables during
                # sanitizer-enabled development builds.  When sanitizers are
                # OFF this path is never taken, so external consumers are
                # unaffected.
                target_link_options(${TARGET} PUBLIC ${_MBOOTCORE_SANITIZER_LINK_FLAGS})
            else()
                target_link_options(${TARGET} PRIVATE ${_MBOOTCORE_SANITIZER_LINK_FLAGS})
            endif()
        endif()
    endif()
endfunction()

# ---------------------------------------------------------------------------
# mbootcore_add_sanitizer_definitions(<target>)
#
# Propagate sanitizer-enabled macros so source code can conditionally
# compile sanitizer-aware paths (e.g., disabling inline instrumentation).
# ---------------------------------------------------------------------------
function(mbootcore_add_sanitizer_definitions TARGET)
    if(MBOOTCORE_ENABLE_ASAN)
        target_compile_definitions(${TARGET} PRIVATE MBOOTCORE_HAVE_ASAN=1)
    endif()
    if(MBOOTCORE_ENABLE_UBSAN)
        target_compile_definitions(${TARGET} PRIVATE MBOOTCORE_HAVE_UBSAN=1)
    endif()
    if(MBOOTCORE_ENABLE_TSAN)
        target_compile_definitions(${TARGET} PRIVATE MBOOTCORE_HAVE_TSAN=1)
    endif()
endfunction()
