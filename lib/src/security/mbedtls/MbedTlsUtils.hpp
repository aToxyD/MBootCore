#pragma once

#ifdef MBOOTCORE_HAVE_MBEDTLS

#include <mbootcore/domain/Error.hpp>
#include <mbootcore/security/SecurityTypes.hpp>

#include <mbedtls/md.h>
#include <mbedtls/pk.h>

#include <memory>
#include <vector>

namespace mbootcore { namespace security { namespace mbedtls_provider {

struct PKContextDeleter {
    void operator()(mbedtls_pk_context* p) const noexcept;
};
using UniquePKContext = std::unique_ptr<mbedtls_pk_context, PKContextDeleter>;

struct MDContextDeleter {
    void operator()(mbedtls_md_context_t* p) const noexcept;
};
using UniqueMDContext = std::unique_ptr<mbedtls_md_context_t, MDContextDeleter>;

const mbedtls_md_info_t* resolveMdInfo(HashAlgorithm algo) noexcept;

ErrorCode translateMbedtlsError(int ret) noexcept;

std::vector<uint8_t> computeDigest(const uint8_t* data, size_t len,
                                   HashAlgorithm algo) noexcept;

int constantTimeCompare(const uint8_t* a, const uint8_t* b, size_t len) noexcept;

} } }

#endif
