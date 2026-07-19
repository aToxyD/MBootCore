# CompilerWarnings.cmake — Centralized warning policy for MBootCore targets.
#
# Provides mbootcore_apply_warnings(<target>) which applies:
#   - Warning flags (-Wall -Wextra -Wpedantic / /W4 /permissive-)
#   - Warnings-as-errors (-Werror / /WX) when enabled
#
# Never apply this function to third-party / external targets.

# ---------------------------------------------------------------------------
# Option
# ---------------------------------------------------------------------------
# Default ON for Debug and CI builds, OFF for Release / packaging.
if(NOT DEFINED MBOOTCORE_WARNINGS_AS_ERRORS)
    if(DEFINED ENV{CI})
        set(_mbootcore_wae_default ON)
    elseif(CMAKE_BUILD_TYPE STREQUAL "Debug" OR CMAKE_BUILD_TYPE STREQUAL "")
        set(_mbootcore_wae_default ON)
    else()
        set(_mbootcore_wae_default OFF)
    endif()
else()
    set(_mbootcore_wae_default ${MBOOTCORE_WARNINGS_AS_ERRORS})
endif()

option(MBOOTCORE_WARNINGS_AS_ERRORS
    "Treat compiler warnings as errors (recommended for Debug and CI builds)"
    ${_mbootcore_wae_default})
unset(_mbootcore_wae_default)

if(MBOOTCORE_WARNINGS_AS_ERRORS)
    message(STATUS "MBootCore: warnings-as-errors ENABLED")
else()
    message(STATUS "MBootCore: warnings-as-errors disabled")
endif()

# ---------------------------------------------------------------------------
# Compiler detection
# ---------------------------------------------------------------------------
include(CheckCXXCompilerFlag)

# ---------------------------------------------------------------------------
# mbootcore_apply_warnings(<target>)
#
# Apply the standard warning policy to a single target.
# Call once per target, after the target is created.
# ---------------------------------------------------------------------------
function(mbootcore_apply_warnings TARGET)
    target_compile_options(${TARGET} PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:/W4 /permissive->
        $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -Wpedantic>
    )

    if(MBOOTCORE_WARNINGS_AS_ERRORS)
        target_compile_options(${TARGET} PRIVATE
            $<$<CXX_COMPILER_ID:MSVC>:/WX>
            $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Werror>
        )
    endif()
endfunction()
