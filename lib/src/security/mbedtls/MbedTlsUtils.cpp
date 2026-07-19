#ifdef MBOOTCORE_HAVE_MBEDTLS

#include "MbedTlsUtils.hpp"
#include <psa/crypto.h>
#include <cstring>

namespace mbootcore { namespace security { namespace mbedtls_provider {

void PKContextDeleter::operator()(mbedtls_pk_context* p) const noexcept {
    if (p) {
        mbedtls_pk_free(p);
        delete p;
    }
}

void MDContextDeleter::operator()(mbedtls_md_context_t* p) const noexcept {
    if (p) {
        mbedtls_md_free(p);
        delete p;
    }
}

const mbedtls_md_info_t* resolveMdInfo(HashAlgorithm algo) noexcept {
    switch (algo) {
    case HashAlgorithm::SHA256:
        return mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
    case HashAlgorithm::SHA512:
        return mbedtls_md_info_from_type(MBEDTLS_MD_SHA512);
    default:
        return nullptr;
    }
}

ErrorCode translateMbedtlsError(int ret) noexcept {
    if (ret == 0) return ErrorCode::Success;
    if (ret == MBEDTLS_ERR_PK_BAD_INPUT_DATA ||
        ret == MBEDTLS_ERR_PK_INVALID_ALG ||
        ret == MBEDTLS_ERR_PK_KEY_INVALID_FORMAT ||
        ret == MBEDTLS_ERR_PK_UNKNOWN_NAMED_CURVE) {
        return ErrorCode::CryptoKeyLoadFailed;
    }
    if (ret == PSA_ERROR_INVALID_SIGNATURE) {
        return ErrorCode::CryptoSignatureVerifyFailed;
    }
    if (ret == MBEDTLS_ERR_MD_BAD_INPUT_DATA) {
        return ErrorCode::InvalidArgument;
    }
    return ErrorCode::CryptoHashFailed;
}

std::vector<uint8_t> computeDigest(const uint8_t* data, size_t len,
                                   HashAlgorithm algo) noexcept {
    const mbedtls_md_info_t* mdInfo = resolveMdInfo(algo);
    if (!mdInfo) return {};

    size_t hashLen = mbedtls_md_get_size(mdInfo);
    std::vector<uint8_t> result(hashLen);

    if (mbedtls_md(mdInfo, data, len, result.data()) != 0) {
        return {};
    }
    return result;
}

int constantTimeCompare(const uint8_t* a, const uint8_t* b, size_t len) noexcept {
    int diff = 0;
    for (size_t i = 0; i < len; ++i) {
        diff |= static_cast<int>(a[i]) ^ static_cast<int>(b[i]);
    }
    return diff;
}

} } }

#endif
