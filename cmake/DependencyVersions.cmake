# ==========================================================================
# DependencyVersions.cmake
# --------------------------------------------------------------------------
# Single source of truth for all third-party dependency versions and
# SHA256 checksums. Every FetchContent/Download declaration must read
# from this file. No version may appear anywhere else.
# ==========================================================================

# ---------------------------------------------------------------------------
# nlohmann/json — header-only JSON library
# ---------------------------------------------------------------------------
set(MBOOTCORE_JSON_VERSION      "v3.12.0")
set(MBOOTCORE_JSON_GITHUB_URL   "https://github.com/nlohmann/json/archive/refs/tags/v3.12.0.tar.gz")
set(MBOOTCORE_JSON_SHA256       "4B92EB0C06D10683F7447CE9406CB97CD4B453BE18D7279320F7B2F025C10187")
set(MBOOTCORE_JSON_EXTRACT_DIR  "json-3.12.0")

# ---------------------------------------------------------------------------
# zlib — compression library (CRC32, compress2)
# ---------------------------------------------------------------------------
set(MBOOTCORE_ZLIB_VERSION      "v1.3.2")
set(MBOOTCORE_ZLIB_GITHUB_URL   "https://github.com/madler/zlib/archive/refs/tags/v1.3.2.tar.gz")
set(MBOOTCORE_ZLIB_SHA256       "B99A0B86C0BA9360EC7E78C4F1E43B1CBDF1E6936C8FA0F6835C0CD694A495A1")
set(MBOOTCORE_ZLIB_EXTRACT_DIR  "zlib-1.3.2")

# ---------------------------------------------------------------------------
# Mbed TLS — cryptographic backend (replaces OpenSSL, no Perl dependency)
# Uses official release asset from GitHub (includes tf-psa-crypto submodule).
# ---------------------------------------------------------------------------
set(MBOOTCORE_MBEDTLS_VERSION      "mbedtls-4.2.0")
set(MBOOTCORE_MBEDTLS_GITHUB_URL   "https://github.com/Mbed-TLS/mbedtls/releases/download/mbedtls-4.2.0/mbedtls-4.2.0.tar.bz2")
set(MBOOTCORE_MBEDTLS_SHA256       "2BED9D713B4668F76553B097E72B8AA30BC8F112A940D7AE228D524BBDE6FFEA")
set(MBOOTCORE_MBEDTLS_EXTRACT_DIR  "mbedtls-4.2.0")

# ---------------------------------------------------------------------------
# libusb — USB device access (optional, gated by MBOOTCORE_ENABLE_USB)
# ---------------------------------------------------------------------------
set(MBOOTCORE_LIBUSB_VERSION      "v1.0.30-0")
set(MBOOTCORE_LIBUSB_GITHUB_URL   "https://github.com/libusb/libusb-cmake/archive/refs/tags/v1.0.30-0.tar.gz")
set(MBOOTCORE_LIBUSB_SHA256       "C8143CEBA42B7B93D484CCD09DB7BE261BB9B15F61DAE5FDC605A01A6913BADA")
set(MBOOTCORE_LIBUSB_EXTRACT_DIR  "libusb-cmake-1.0.30-0")

# ---------------------------------------------------------------------------
# Catch2 — test framework (only when MBOOTCORE_BUILD_TESTS=ON)
# ---------------------------------------------------------------------------
set(MBOOTCORE_CATCH2_VERSION      "v3.15.2")
set(MBOOTCORE_CATCH2_GITHUB_URL   "https://github.com/catchorg/Catch2/archive/refs/tags/v3.15.2.tar.gz")
set(MBOOTCORE_CATCH2_SHA256       "ACFAE120892C2B67A74142D36D060C0CAA96F1C3AAA8AABD96E19961163D0420")
set(MBOOTCORE_CATCH2_EXTRACT_DIR  "Catch2-3.15.2")
