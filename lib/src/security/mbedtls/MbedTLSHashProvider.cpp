#ifdef MBOOTCORE_HAVE_MBEDTLS

#include "mbootcore/security/MbedTLSHashProvider.hpp"
#include "MbedTlsUtils.hpp"

namespace mbootcore { namespace security {

class MbedTLSHashProvider final : public IHashProvider {
public:
    Result<std::vector<uint8_t>> hash(const std::vector<uint8_t>& data,
                                      HashAlgorithm algo) const override {
        const mbedtls_md_info_t* mdInfo =
            mbedtls_provider::resolveMdInfo(algo);
        if (!mdInfo) {
            return Result<std::vector<uint8_t>>::Error(
                ErrorCode::InvalidArgument);
        }

        size_t hashLen = mbedtls_md_get_size(mdInfo);
        std::vector<uint8_t> result(hashLen);

        if (mbedtls_md(mdInfo, data.data(), data.size(), result.data()) != 0) {
            return Result<std::vector<uint8_t>>::Error(
                ErrorCode::CryptoHashFailed);
        }

        return Result<std::vector<uint8_t>>::Ok(std::move(result));
    }
};

std::unique_ptr<IHashProvider> makeMbedTLSHashProvider() {
    return std::make_unique<MbedTLSHashProvider>();
}

} }

#endif
