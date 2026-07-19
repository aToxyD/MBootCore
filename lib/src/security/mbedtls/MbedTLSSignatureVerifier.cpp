#ifdef MBOOTCORE_HAVE_MBEDTLS

#include "mbootcore/security/MbedTLSSignatureVerifier.hpp"
#include "MbedTlsUtils.hpp"

#include <fstream>
#include <mutex>
#include <cstring>
#include <mbedtls/pk.h>

namespace mbootcore { namespace security {

class MbedTLSSignatureVerifier final : public ISignatureVerifier {
public:
    Result<SignatureVerificationResult> verify(
        const std::vector<uint8_t>& data,
        const std::vector<uint8_t>& signature) override {

        if (data.empty() || signature.empty()) {
            return Result<SignatureVerificationResult>::Ok(
                SignatureVerificationResult::NotSigned);
        }

        return verifyRaw(data, signature);
    }

    Result<SignatureVerificationResult> verifyFile(
        const std::string& filePath) override {

        std::ifstream file(filePath, std::ios::binary);
        if (!file.is_open()) {
            return Result<SignatureVerificationResult>::Error(
                ErrorCode::InvalidArgument);
        }

        std::vector<uint8_t> fileData(
            (std::istreambuf_iterator<char>(file)),
            std::istreambuf_iterator<char>());

        std::string sigPath = filePath + ".sig";
        std::ifstream sigFile(sigPath, std::ios::binary);
        if (!sigFile.is_open()) {
            return Result<SignatureVerificationResult>::Ok(
                SignatureVerificationResult::NotSigned);
        }

        std::vector<uint8_t> sigData(
            (std::istreambuf_iterator<char>(sigFile)),
            std::istreambuf_iterator<char>());

        return verifyRaw(fileData, sigData);
    }

    void setPublicKeyPem(const std::string& pem) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_publicKeyPem = pem;
        m_cachedKey.reset();
    }

    void setPublicKeyData(const std::vector<uint8_t>& derData) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_publicKeyDer = derData;
        m_cachedKey.reset();
    }

private:
    Result<SignatureVerificationResult> verifyRaw(
        const std::vector<uint8_t>& data,
        const std::vector<uint8_t>& signature) {

        auto key = loadPublicKey();
        if (!key) {
            return Result<SignatureVerificationResult>::Ok(
                SignatureVerificationResult::NotSupported);
        }

        auto computed = mbedtls_provider::computeDigest(
            data.data(), data.size(), HashAlgorithm::SHA256);
        if (computed.empty()) {
            return Result<SignatureVerificationResult>::Error(
                ErrorCode::CryptoHashFailed);
        }

        int ret = mbedtls_pk_verify(key, MBEDTLS_MD_SHA256,
                                     computed.data(), computed.size(),
                                     signature.data(), signature.size());

        if (ret == 0) {
            return Result<SignatureVerificationResult>::Ok(
                SignatureVerificationResult::Verified);
        }

        return Result<SignatureVerificationResult>::Ok(
            SignatureVerificationResult::Failed);
    }

    mbedtls_pk_context* loadPublicKey() {
        std::lock_guard<std::mutex> lock(m_mutex);

        if (m_cachedKey) {
            return m_cachedKey.get();
        }

        auto ctx = mbedtls_provider::UniquePKContext(new mbedtls_pk_context());
        mbedtls_pk_init(ctx.get());

        bool loaded = false;
        if (!m_publicKeyPem.empty()) {
            loaded = (mbedtls_pk_parse_public_key(ctx.get(),
                reinterpret_cast<const uint8_t*>(m_publicKeyPem.data()),
                m_publicKeyPem.size() + 1) == 0);
        } else if (!m_publicKeyDer.empty()) {
            loaded = (mbedtls_pk_parse_public_key(ctx.get(),
                m_publicKeyDer.data(), m_publicKeyDer.size()) == 0);
        }

        if (loaded) {
            m_cachedKey = std::move(ctx);
            return m_cachedKey.get();
        }

        return nullptr;
    }

    std::string m_publicKeyPem;
    std::vector<uint8_t> m_publicKeyDer;
    mbedtls_provider::UniquePKContext m_cachedKey;
    mutable std::mutex m_mutex;
};

std::unique_ptr<ISignatureVerifier> makeMbedTLSSignatureVerifier() {
    return std::make_unique<MbedTLSSignatureVerifier>();
}

void setPublicKeyPem(ISignatureVerifier& verifier, const std::string& pem) {
    static_cast<MbedTLSSignatureVerifier&>(verifier).setPublicKeyPem(pem);
}

} }

#endif
