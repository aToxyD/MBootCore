#ifdef MBOOTCORE_HAVE_MBEDTLS

#include "mbootcore/security/MbedTLSIntegrityVerifier.hpp"
#include "MbedTlsUtils.hpp"

#include <fstream>
#include <sstream>

namespace mbootcore { namespace security {

class MbedTLSIntegrityVerifier final : public IIntegrityVerifier {
public:
    Result<bool> isTampered(const std::string& path) override {
        auto result = verifyIntegrity(path);
        if (result.isError()) {
            return Result<bool>::Error(result.error());
        }
        return Result<bool>::Ok(!result.value());
    }

    Result<bool> verifyIntegrity(const std::string& path) override {
        std::ifstream file(path, std::ios::binary);
        if (!file.is_open()) {
            return Result<bool>::Error(ErrorCode::InvalidArgument);
        }

        std::vector<uint8_t> fileData(
            (std::istreambuf_iterator<char>(file)),
            std::istreambuf_iterator<char>());

        std::string hashPath = path + ".sha256";
        std::ifstream hashFile(hashPath);
        if (!hashFile.is_open()) {
            return Result<bool>::Error(ErrorCode::NotSupported);
        }

        std::string expectedHex;
        std::getline(hashFile, expectedHex);

        auto computed = mbedtls_provider::computeDigest(
            fileData.data(), fileData.size(), HashAlgorithm::SHA256);

        if (computed.empty()) {
            return Result<bool>::Error(ErrorCode::CryptoHashFailed);
        }

        std::string computedHex = toHex(computed);
        if (computedHex.size() >= expectedHex.size()) {
            computedHex = computedHex.substr(computedHex.size() - expectedHex.size());
        }

        if (computedHex != expectedHex) {
            return Result<bool>::Ok(false);
        }

        return Result<bool>::Ok(true);
    }

private:
    static std::string toHex(const std::vector<uint8_t>& data) {
        static const char hexChars[] = "0123456789abcdef";
        std::string result;
        result.reserve(data.size() * 2);
        for (uint8_t byte : data) {
            result.push_back(hexChars[(byte >> 4) & 0x0F]);
            result.push_back(hexChars[byte & 0x0F]);
        }
        return result;
    }
};

std::unique_ptr<IIntegrityVerifier> makeMbedTLSIntegrityVerifier() {
    return std::make_unique<MbedTLSIntegrityVerifier>();
}

} }

#endif
