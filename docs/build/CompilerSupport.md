# Compiler Support

## Supported Compilers

| Compiler | Min Version | Max Version | Status |
|----------|-------------|-------------|--------|
| GCC | 11 | Latest | ✅ Tier 1 |
| Clang | 14 | Latest | ⚠️ Tier 2 |
| MSVC | Visual Studio 2022 17.x | 2022 | ⚠️ Tier 3 |

## Tier 1: GCC

GCC is the primary compiler used in development and release packaging.

### Minimum: GCC 11

- Full C++17 support
- Structured bindings
- `std::optional`, `std::variant`, `std::string_view`
- `if constexpr`
- Fold expressions
- `std::filesystem`

### Tested Versions

- GCC 11 (standard target for Debian/Ubuntu LTS)
- GCC 14 (latest)

## Tier 2: Clang

Clang is tested quarterly and validated for macOS + Linux builds.

### Minimum: Clang 15

- Full C++17 support
- Apple Clang (Xcode 15+) on macOS

### Notes

- Clang on Windows is not tested
- Apple Clang versioning differs from upstream Clang (Xcode 15 includes Clang 15)

## Tier 3: MSVC

MSVC is tested and used for release packaging on Windows.

### Minimum: Visual Studio 2022 17.x

- `/std:c++17` flag required
- `/permissive-` flag recommended for standards conformance

### Status

MSVC builds and tests pass on Windows (`windows-msvc` matrix in release workflow).
Known issues with:
- `constexpr` string operations
- `std::filesystem::path` edge cases
- Structured binding in certain contexts

## Build Configuration

### GCC / Clang

```cmake
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
```

### MSVC

```cmake
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++17 /permissive- /Zc:__cplusplus")
```

### Compiler Warnings

MBootCore centralizes all warning configuration in `cmake/CompilerWarnings.cmake`.
The `mbootcore_apply_warnings(<target>)` function applies the appropriate flags
per compiler and optionally enables warnings-as-errors (`-Werror` / `/WX`).

| Compiler | Warning flags | Warnings-as-errors |
|----------|---------------|--------------------|
| GCC / Clang | `-Wall -Wextra -Wpedantic` | `-Werror` |
| MSVC | `/W4 /permissive-` | `/WX` |

Warnings-as-errors are controlled by `MBOOTCORE_WARNINGS_AS_ERRORS` (see
[BUILD_OPTIONS.md](../BUILD_OPTIONS.md)). Third-party dependencies are
never affected.

## Feature Detection

The build system uses CMake's `CheckCXXCompilerFlag` to detect compiler capabilities:

```cmake
include(CheckCXXCompilerFlag)
check_cxx_compiler_flag("-fno-exceptions" COMPILER_HAS_FNO_EXCEPTIONS)
```

## Preprocessor Macros

| Macro | When Defined |
|-------|-------------|
| `MBOOTCORE_COMPILER_GCC` | GCC |
| `MBOOTCORE_COMPILER_CLANG` | Clang |
| `MBOOTCORE_COMPILER_MSVC` | MSVC |
