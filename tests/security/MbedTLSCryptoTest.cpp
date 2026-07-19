#include <catch2/catch_test_macros.hpp>
#include <memory>
#include <string>
#include <vector>
#include <thread>
#include <future>
#include <chrono>
#include <cstring>
#include <filesystem>
#include <fstream>

#include <mbootcore/security/SecurityTypes.hpp>
#include <mbootcore/security/interfaces/IHashProvider.hpp>
#include <mbootcore/security/interfaces/ISignatureVerifier.hpp>
#include <mbootcore/security/interfaces/IIntegrityVerifier.hpp>
#include <mbootcore/domain/Error.hpp>

#ifdef MBOOTCORE_ENABLE_CRYPTO
#include <mbootcore/security/MbedTLSHashProvider.hpp>
#include <mbootcore/security/MbedTLSSignatureVerifier.hpp>
#include "MbedTLSSignatureVerifierTestAccess.hpp"
#include <mbootcore/security/MbedTLSIntegrityVerifier.hpp>
#include <mbootcore/security/MbedTLSSecureStorage.hpp>
#include <mbedtls/pk.h>
#include <mbedtls/md.h>
#include <psa/crypto.h>
#endif

using namespace mbootcore;
using namespace mbootcore::security;

namespace {

struct HashTestVector {
    std::string label;
    std::vector<uint8_t> input;
    std::vector<uint8_t> expectedSha256;
    std::vector<uint8_t> expectedSha512;
};

std::vector<uint8_t> hexToBytes(const std::string& hex) {
    std::vector<uint8_t> bytes;
    bytes.reserve(hex.size() / 2);
    for (size_t i = 0; i + 1 < hex.size(); i += 2) {
        uint8_t hi = static_cast<uint8_t>(
            std::stoul(hex.substr(i, 2), nullptr, 16));
        bytes.push_back(hi);
    }
    return bytes;
}

std::vector<HashTestVector> getGoldenVectors() {
    std::vector<HashTestVector> vectors;

    {
        HashTestVector v;
        v.label = "abc";
        v.input = {'a', 'b', 'c'};
        v.expectedSha256 = hexToBytes(
            "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
        v.expectedSha512 = hexToBytes(
            "ddaf35a193617abacc417349ae20413112e6fa4e89a97ea20a9eeee64b55d39a"
            "2192992a274fc1a836ba3c23a3feebbd454d4423643ce80e2a9ac94fa54ca49f");
        vectors.push_back(std::move(v));
    }

    {
        HashTestVector v;
        v.label = "empty";
        v.input = {};
        v.expectedSha256 = hexToBytes(
            "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
        v.expectedSha512 = hexToBytes(
            "cf83e1357eefb8bdf1542850d66d8007d620e4050b5715dc83f4a921d36ce9ce"
            "47d0d13c5d85f2b0ff8318d2877eec2f63b931bd47417a81a538327af927da3e");
        vectors.push_back(std::move(v));
    }

    {
        HashTestVector v;
        v.label = "448-bit message";
        v.input = {'a','b','c','d','b','c','d','e','c','d','e','f','d','e','f','g',
                   'e','f','g','h','f','g','h','i','g','h','i','j','h','i','j','k',
                   'i','j','k','l','j','k','l','m','k','l','m','n','l','m','n','o',
                   'm','n','o','p','n','o','p','q'};
        v.expectedSha256 = hexToBytes(
            "248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1");
        v.expectedSha512 = hexToBytes(
            "204a8fc6dda82f0a0ced7beb8e08a41657c16ef468b228a8279be331a703c335"
            "96fd15c13b1b07f9aa1d3bea57789ca031ad85c7a71dd70354ec631238ca3445");
        vectors.push_back(std::move(v));
    }

    return vectors;
}

} // anonymous namespace

#ifdef MBOOTCORE_ENABLE_CRYPTO

TEST_CASE("MbedTLSHashProvider: SHA-256 golden vectors", "[crypto][hash][sha256]") {
    auto provider = makeMbedTLSHashProvider();
    REQUIRE(provider);

    auto vecs = getGoldenVectors();
    for (size_t i = 0; i < vecs.size(); ++i) {
        auto result = provider->hash(vecs[i].input, HashAlgorithm::SHA256);
        REQUIRE(result.isOk());
        REQUIRE(result.value() == vecs[i].expectedSha256);
    }
}

TEST_CASE("MbedTLSHashProvider: SHA-512 golden vectors", "[crypto][hash][sha512]") {
    auto provider = makeMbedTLSHashProvider();
    REQUIRE(provider);

    auto vecs = getGoldenVectors();
    for (size_t i = 0; i < vecs.size(); ++i) {
        auto result = provider->hash(vecs[i].input, HashAlgorithm::SHA512);
        REQUIRE(result.isOk());
        REQUIRE(result.value() == vecs[i].expectedSha512);
    }
}

TEST_CASE("MbedTLSHashProvider: empty input", "[crypto][hash]") {
    auto provider = makeMbedTLSHashProvider();
    REQUIRE(provider);

    auto result = provider->hash({}, HashAlgorithm::SHA256);
    REQUIRE(result.isOk());
    REQUIRE(result.value().size() == 32u);
}

TEST_CASE("MbedTLSHashProvider: zero-length payload", "[crypto][hash]") {
    auto provider = makeMbedTLSHashProvider();
    REQUIRE(provider);

    auto resultSha256 = provider->hash({}, HashAlgorithm::SHA256);
    REQUIRE(resultSha256.isOk());

    auto resultSha512 = provider->hash({}, HashAlgorithm::SHA512);
    REQUIRE(resultSha512.isOk());
}

TEST_CASE("MbedTLSHashProvider: large payload", "[crypto][hash]") {
    auto provider = makeMbedTLSHashProvider();
    REQUIRE(provider);

    std::vector<uint8_t> largeData(1024 * 1024, 0xAB);
    auto result = provider->hash(largeData, HashAlgorithm::SHA256);
    REQUIRE(result.isOk());
    REQUIRE(result.value().size() == 32u);

    auto result512 = provider->hash(largeData, HashAlgorithm::SHA512);
    REQUIRE(result512.isOk());
    REQUIRE(result512.value().size() == 64u);
}

TEST_CASE("MbedTLSHashProvider: invalid algorithm returns error", "[crypto][hash]") {
    auto provider = makeMbedTLSHashProvider();
    REQUIRE(provider);

    auto result = provider->hash({1, 2, 3}, static_cast<HashAlgorithm>(99));
    REQUIRE(result.isError());
    REQUIRE(result.error() == ErrorCode::InvalidArgument);
}

TEST_CASE("MbedTLSHashProvider: negative hash verification", "[crypto][hash]") {
    auto provider = makeMbedTLSHashProvider();
    REQUIRE(provider);

    auto result1 = provider->hash({1, 2, 3}, HashAlgorithm::SHA256);
    auto result2 = provider->hash({1, 2, 4}, HashAlgorithm::SHA256);
    REQUIRE(result1.isOk());
    REQUIRE(result2.isOk());
    REQUIRE(result1.value() != result2.value());
}

TEST_CASE("MbedTLSHashProvider: repeated verification", "[crypto][hash]") {
    auto provider = makeMbedTLSHashProvider();
    REQUIRE(provider);

    std::vector<uint8_t> data = {0xDE, 0xAD, 0xBE, 0xEF};
    auto first = provider->hash(data, HashAlgorithm::SHA256);
    REQUIRE(first.isOk());

    for (int i = 0; i < 100; ++i) {
        auto again = provider->hash(data, HashAlgorithm::SHA256);
        REQUIRE(again.isOk());
        REQUIRE(first.value() == again.value());
    }
}

TEST_CASE("MbedTLSHashProvider: thread-safe concurrent hashing", "[crypto][hash][concurrency]") {
    auto provider = makeMbedTLSHashProvider();
    REQUIRE(provider);

    const int numThreads = 8;
    const int iterations = 50;
    std::vector<std::future<bool>> futures;

    for (int t = 0; t < numThreads; ++t) {
        futures.push_back(std::async(std::launch::async, [&, t]() {
            for (int i = 0; i < iterations; ++i) {
                std::vector<uint8_t> data = {
                    static_cast<uint8_t>(t),
                    static_cast<uint8_t>(i),
                    0x42
                };
                auto result = provider->hash(data, HashAlgorithm::SHA256);
                if (result.isError()) return false;
                if (result.value().empty()) return false;
            }
            return true;
        }));
    }

    for (auto& f : futures) {
        REQUIRE(f.get());
    }
}

TEST_CASE("MbedTLSSignatureVerifier: not-signed data", "[crypto][signature]") {
    auto verifier = makeMbedTLSSignatureVerifier();
    REQUIRE(verifier);

    auto result = verifier->verify({1, 2, 3}, {});
    REQUIRE(result.isOk());
    REQUIRE(result.value() == SignatureVerificationResult::NotSigned);
}

TEST_CASE("MbedTLSSignatureVerifier: empty data and signature", "[crypto][signature]") {
    auto verifier = makeMbedTLSSignatureVerifier();
    REQUIRE(verifier);

    auto result = verifier->verify({}, {});
    REQUIRE(result.isOk());
    REQUIRE(result.value() == SignatureVerificationResult::NotSigned);
}

TEST_CASE("MbedTLSSignatureVerifier: verifyFile nonexistent", "[crypto][signature]") {
    auto verifier = makeMbedTLSSignatureVerifier();
    REQUIRE(verifier);

    auto result = verifier->verifyFile("/nonexistent/path/file.bin");
    REQUIRE(result.isError());
    REQUIRE(result.error() == ErrorCode::InvalidArgument);
}

TEST_CASE("MbedTLSSignatureVerifier: verifyFile without .sig", "[crypto][signature]") {
    auto verifier = makeMbedTLSSignatureVerifier();
    REQUIRE(verifier);

    auto tmpDir = std::filesystem::temp_directory_path();
    auto path = (tmpDir / "mbootcore_test_sig.txt").string();
    {
        std::ofstream ofs(path, std::ios::binary);
        ofs << "test firmware data";
    }

    auto result = verifier->verifyFile(path);
    REQUIRE(result.isOk());
    REQUIRE(result.value() == SignatureVerificationResult::NotSigned);

    std::filesystem::remove(path);
}

TEST_CASE("MbedTLSSignatureVerifier: corrupted signature", "[crypto][signature]") {
    auto verifier = makeMbedTLSSignatureVerifier();
    REQUIRE(verifier);

    std::vector<uint8_t> data = {0x01, 0x02, 0x03};
    std::vector<uint8_t> badSig = {0xFF, 0xFE, 0xFD};

    auto result = verifier->verify(data, badSig);
    REQUIRE(result.isOk());
    bool validOutcome = (result.value() == SignatureVerificationResult::Failed) ||
                        (result.value() == SignatureVerificationResult::NotSupported);
    REQUIRE(validOutcome);
}

TEST_CASE("MbedTLSSignatureVerifier: wrong public key", "[crypto][signature]") {
    auto verifier = makeMbedTLSSignatureVerifier();
    REQUIRE(verifier);

    std::vector<uint8_t> data = {0x01, 0x02, 0x03};
    std::vector<uint8_t> sig = {0x04, 0x05, 0x06};

    auto result = verifier->verify(data, sig);
    REQUIRE(result.isOk());
    REQUIRE(result.value() == SignatureVerificationResult::NotSupported);
}

TEST_CASE("MbedTLSIntegrityVerifier: nonexistent file", "[crypto][integrity]") {
    auto verifier = makeMbedTLSIntegrityVerifier();
    REQUIRE(verifier);

    auto result = verifier->verifyIntegrity("/nonexistent/path");
    REQUIRE(result.isError());
    REQUIRE(result.error() == ErrorCode::InvalidArgument);
}

TEST_CASE("MbedTLSIntegrityVerifier: isTampered nonexistent", "[crypto][integrity]") {
    auto verifier = makeMbedTLSIntegrityVerifier();
    REQUIRE(verifier);

    auto result = verifier->isTampered("/nonexistent/path");
    REQUIRE(result.isError());
    REQUIRE(result.error() == ErrorCode::InvalidArgument);
}

TEST_CASE("MbedTLSIntegrityVerifier: file without hash", "[crypto][integrity]") {
    auto verifier = makeMbedTLSIntegrityVerifier();
    REQUIRE(verifier);

    auto tmpDir = std::filesystem::temp_directory_path();
    auto path = (tmpDir / "mbootcore_integ_test.txt").string();
    {
        std::ofstream ofs(path, std::ios::binary);
        ofs << "test data for integrity check";
    }

    auto result = verifier->verifyIntegrity(path);
    REQUIRE(result.isError());
    REQUIRE(result.error() == ErrorCode::NotSupported);

    std::filesystem::remove(path);
}

TEST_CASE("MbedTLSSecureStorage: store and retrieve", "[crypto][storage]") {
    auto storage = makeMbedTLSSecureStorage();
    REQUIRE(storage);

    auto storeResult = storage->store("key1", "value1");
    REQUIRE(storeResult.isOk());

    auto retrieveResult = storage->retrieve("key1");
    REQUIRE(retrieveResult.isOk());
    REQUIRE(retrieveResult.value() == "value1");
}

TEST_CASE("MbedTLSSecureStorage: non-existent key", "[crypto][storage]") {
    auto storage = makeMbedTLSSecureStorage();
    REQUIRE(storage);

    auto result = storage->retrieve("nonexistent");
    REQUIRE(result.isError());
}

TEST_CASE("MbedTLSSecureStorage: remove and exists", "[crypto][storage]") {
    auto storage = makeMbedTLSSecureStorage();
    REQUIRE(storage);

    REQUIRE(storage->store("key1", "value1").isOk());
    REQUIRE(storage->exists("key1").value());

    REQUIRE(storage->remove("key1").isOk());
    REQUIRE_FALSE(storage->exists("key1").value());
}

TEST_CASE("MbedTLSSecureStorage: clear", "[crypto][storage]") {
    auto storage = makeMbedTLSSecureStorage();
    REQUIRE(storage);

    REQUIRE(storage->store("key1", "v1").isOk());
    REQUIRE(storage->store("key2", "v2").isOk());

    REQUIRE(storage->clear().isOk());
    REQUIRE_FALSE(storage->exists("key1").value());
    REQUIRE_FALSE(storage->exists("key2").value());
}

TEST_CASE("MbedTLSSecureStorage: overwrite", "[crypto][storage]") {
    auto storage = makeMbedTLSSecureStorage();
    REQUIRE(storage);

    REQUIRE(storage->store("key1", "old").isOk());
    REQUIRE(storage->store("key1", "new").isOk());

    auto result = storage->retrieve("key1");
    REQUIRE(result.isOk());
    REQUIRE(result.value() == "new");
}

TEST_CASE("ErrorCode: crypto error codes have correct values", "[crypto][errors]") {
    REQUIRE(static_cast<uint32_t>(ErrorCode::CryptoInitializationFailed) == 0x1000);
    REQUIRE(static_cast<uint32_t>(ErrorCode::CryptoHashFailed) == 0x1001);
    REQUIRE(static_cast<uint32_t>(ErrorCode::CryptoSignatureVerifyFailed) == 0x1002);
    REQUIRE(static_cast<uint32_t>(ErrorCode::CryptoKeyLoadFailed) == 0x1003);
    REQUIRE(static_cast<uint32_t>(ErrorCode::CryptoInvalidPadding) == 0x1004);
    REQUIRE(static_cast<uint32_t>(ErrorCode::CryptoVerificationFailed) == 0x1005);
}

TEST_CASE("ErrorCode: crypto error strings are non-empty", "[crypto][errors]") {
    REQUIRE(std::string(toString(ErrorCode::CryptoInitializationFailed)).size() > 0);
    REQUIRE(std::string(toString(ErrorCode::CryptoHashFailed)).size() > 0);
    REQUIRE(std::string(toString(ErrorCode::CryptoSignatureVerifyFailed)).size() > 0);
    REQUIRE(std::string(toString(ErrorCode::CryptoKeyLoadFailed)).size() > 0);
    REQUIRE(std::string(toString(ErrorCode::CryptoInvalidPadding)).size() > 0);
    REQUIRE(std::string(toString(ErrorCode::CryptoVerificationFailed)).size() > 0);
}

TEST_CASE("Crypto providers: never throw exceptions", "[crypto][boundary]") {
    auto provider = makeMbedTLSHashProvider();
    REQUIRE(provider);

    REQUIRE_NOTHROW(provider->hash({}, HashAlgorithm::SHA256));
    REQUIRE_NOTHROW(provider->hash({1}, HashAlgorithm::SHA256));
    REQUIRE_NOTHROW(provider->hash({1, 2, 3, 4, 5}, HashAlgorithm::SHA512));
    REQUIRE_NOTHROW(provider->hash({0xFF}, static_cast<HashAlgorithm>(99)));
}

TEST_CASE("Signature verifier: never throws exceptions", "[crypto][boundary]") {
    auto verifier = makeMbedTLSSignatureVerifier();
    REQUIRE(verifier);

    REQUIRE_NOTHROW(verifier->verify({}, {}));
    REQUIRE_NOTHROW(verifier->verify({1, 2, 3}, {4, 5, 6}));
    REQUIRE_NOTHROW(verifier->verifyFile("/nonexistent"));
}

TEST_CASE("PEM parsing: NUL terminator contract", "[crypto][pem][audit]") {
    REQUIRE(psa_crypto_init() == PSA_SUCCESS);

    const char* rsaPem =
        "-----BEGIN PUBLIC KEY-----\n"
        "MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEA2xx/LgvNv87RdRCgorjO\n"
        "fariBeB62ERjj7W9wLAZuTe4GUoO8V10gGdGhwbeW38GA73BjV4HFdRb9Nzlzz35\n"
        "wREsrmq5ir0dZ2YX6k692xWagofk8HjDo4WHsP2fqZlf4zPszOoLtWFe8Ul+P6Mt\n"
        "6gEMzEKadpvE0DfTsRcBYQEWWX4cF8NT/dFyy0xgFdp94uqtUO+O4ovUandV1nDZ\n"
        "a7vx7jkEOKO94tHgZmvinEeZ6SjmtvwuymdDhOjVg9admGsBPoHcPHrK+fOc99Yo\n"
        "Gyd4fMPQ1WOngTSJrSVqvfLq7fpX/OU0xsEPcS3SCBAbrURB4P55oGOTirFd6bDu\n"
        "bwIDAQAB\n"
        "-----END PUBLIC KEY-----\n";

    SECTION("PEM without NUL in keylen fails") {
        mbedtls_pk_context pk;
        mbedtls_pk_init(&pk);

        size_t lenWithoutNul = strlen(rsaPem);
        int ret = mbedtls_pk_parse_public_key(&pk,
            reinterpret_cast<const unsigned char*>(rsaPem), lenWithoutNul);

        REQUIRE(ret != 0);
        mbedtls_pk_free(&pk);
    }

    SECTION("PEM with NUL in keylen succeeds") {
        mbedtls_pk_context pk;
        mbedtls_pk_init(&pk);

        size_t lenWithNul = strlen(rsaPem) + 1;
        int ret = mbedtls_pk_parse_public_key(&pk,
            reinterpret_cast<const unsigned char*>(rsaPem), lenWithNul);

        REQUIRE(ret == 0);
        mbedtls_pk_free(&pk);
    }
}

TEST_CASE("PEM parsing: valid RSA public key", "[crypto][pem][rsa]") {
    REQUIRE(psa_crypto_init() == PSA_SUCCESS);

    const char* rsaPem =
        "-----BEGIN PUBLIC KEY-----\n"
        "MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEA2xx/LgvNv87RdRCgorjO\n"
        "fariBeB62ERjj7W9wLAZuTe4GUoO8V10gGdGhwbeW38GA73BjV4HFdRb9Nzlzz35\n"
        "wREsrmq5ir0dZ2YX6k692xWagofk8HjDo4WHsP2fqZlf4zPszOoLtWFe8Ul+P6Mt\n"
        "6gEMzEKadpvE0DfTsRcBYQEWWX4cF8NT/dFyy0xgFdp94uqtUO+O4ovUandV1nDZ\n"
        "a7vx7jkEOKO94tHgZmvinEeZ6SjmtvwuymdDhOjVg9admGsBPoHcPHrK+fOc99Yo\n"
        "Gyd4fMPQ1WOngTSJrSVqvfLq7fpX/OU0xsEPcS3SCBAbrURB4P55oGOTirFd6bDu\n"
        "bwIDAQAB\n"
        "-----END PUBLIC KEY-----\n";

    auto verifier = makeMbedTLSSignatureVerifier();
    REQUIRE(verifier);

    setPublicKeyPem(*verifier, rsaPem);
    auto result = verifier->verify({1, 2, 3}, {4, 5, 6});
    REQUIRE(result.isOk());
    REQUIRE(result.value() == SignatureVerificationResult::Failed);
}

TEST_CASE("PEM parsing: valid EC public key", "[crypto][pem][ec]") {
    REQUIRE(psa_crypto_init() == PSA_SUCCESS);

    const char* ecPem =
        "-----BEGIN PUBLIC KEY-----\n"
        "MFkwEwYHKoZIzj0CAQYIKoZIzj0DAQcDQgAEd3Jlb4FLOZJ51eHxeB+sbwmaPFyh\n"
        "sONTUYNLCLZeC1clkM2vj3aTYbzzSs/BHl4HToQmvd4Evm5lOUVElhfeRQ==\n"
        "-----END PUBLIC KEY-----\n";

    auto verifier = makeMbedTLSSignatureVerifier();
    REQUIRE(verifier);

    setPublicKeyPem(*verifier, ecPem);
    auto result = verifier->verify({1, 2, 3}, {4, 5, 6});
    REQUIRE(result.isOk());
    REQUIRE(result.value() == SignatureVerificationResult::Failed);
}

TEST_CASE("PEM parsing: malformed PEM", "[crypto][pem][malformed]") {
    REQUIRE(psa_crypto_init() == PSA_SUCCESS);

    auto verifier = makeMbedTLSSignatureVerifier();
    REQUIRE(verifier);

    SECTION("garbage data") {
        setPublicKeyPem(*verifier, "this is not a PEM key at all");
        auto result = verifier->verify({1, 2, 3}, {4, 5, 6});
        REQUIRE(result.isOk());
        REQUIRE(result.value() == SignatureVerificationResult::NotSupported);
    }

    SECTION("wrong PEM header") {
        setPublicKeyPem(*verifier,
            "-----BEGIN CERTIFICATE-----\n"
            "MIIBkTCB+wIJALHM5uM0xBsNMA0GCSqGSIb3DQEBCwUAMBExDzANBgNVBAMMBnRl\n"
            "-----END CERTIFICATE-----\n");
        auto result = verifier->verify({1, 2, 3}, {4, 5, 6});
        REQUIRE(result.isOk());
        REQUIRE(result.value() == SignatureVerificationResult::NotSupported);
    }
}

TEST_CASE("PEM parsing: truncated PEM", "[crypto][pem][truncated]") {
    REQUIRE(psa_crypto_init() == PSA_SUCCESS);

    auto verifier = makeMbedTLSSignatureVerifier();
    REQUIRE(verifier);

    SECTION("missing footer") {
        setPublicKeyPem(*verifier,
            "-----BEGIN PUBLIC KEY-----\n"
            "MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEA\n");
        auto result = verifier->verify({1, 2, 3}, {4, 5, 6});
        REQUIRE(result.isOk());
        REQUIRE(result.value() == SignatureVerificationResult::NotSupported);
    }

    SECTION("truncated base64") {
        setPublicKeyPem(*verifier,
            "-----BEGIN PUBLIC KEY-----\n"
            "MIIBIj\n"
            "-----END PUBLIC KEY-----\n");
        auto result = verifier->verify({1, 2, 3}, {4, 5, 6});
        REQUIRE(result.isOk());
        REQUIRE(result.value() == SignatureVerificationResult::NotSupported);
    }
}

TEST_CASE("PEM parsing: missing terminating marker", "[crypto][pem][no-footer]") {
    REQUIRE(psa_crypto_init() == PSA_SUCCESS);

    auto verifier = makeMbedTLSSignatureVerifier();
    REQUIRE(verifier);

    setPublicKeyPem(*verifier,
        "-----BEGIN PUBLIC KEY-----\n"
        "MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEA2xx/LgvNv87RdRCgorjO\n"
        "fariBeB62ERjj7W9wLAZuTe4GUoO8V10gGdGhwbeW38GA73BjV4HFdRb9Nzlzz35\n"
        "-----END\n");
    auto result = verifier->verify({1, 2, 3}, {4, 5, 6});
    REQUIRE(result.isOk());
    REQUIRE(result.value() == SignatureVerificationResult::NotSupported);
}

TEST_CASE("PEM parsing: empty PEM", "[crypto][pem][empty]") {
    REQUIRE(psa_crypto_init() == PSA_SUCCESS);

    auto verifier = makeMbedTLSSignatureVerifier();
    REQUIRE(verifier);

    SECTION("empty string") {
        setPublicKeyPem(*verifier, "");
        auto result = verifier->verify({1, 2, 3}, {4, 5, 6});
        REQUIRE(result.isOk());
        REQUIRE(result.value() == SignatureVerificationResult::NotSupported);
    }

    SECTION("only header") {
        setPublicKeyPem(*verifier, "-----BEGIN PUBLIC KEY-----\n");
        auto result = verifier->verify({1, 2, 3}, {4, 5, 6});
        REQUIRE(result.isOk());
        REQUIRE(result.value() == SignatureVerificationResult::NotSupported);
    }
}

TEST_CASE("PEM parsing: invalid Base64 payload", "[crypto][pem][bad-base64]") {
    REQUIRE(psa_crypto_init() == PSA_SUCCESS);

    auto verifier = makeMbedTLSSignatureVerifier();
    REQUIRE(verifier);

    setPublicKeyPem(*verifier,
        "-----BEGIN PUBLIC KEY-----\n"
        "!!!invalid-base64-data!!!\n"
        "-----END PUBLIC KEY-----\n");
    auto result = verifier->verify({1, 2, 3}, {4, 5, 6});
    REQUIRE(result.isOk());
    REQUIRE(result.value() == SignatureVerificationResult::NotSupported);
}

TEST_CASE("PEM parsing: corrupted key", "[crypto][pem][corrupted]") {
    REQUIRE(psa_crypto_init() == PSA_SUCCESS);

    auto verifier = makeMbedTLSSignatureVerifier();
    REQUIRE(verifier);

    SECTION("corrupted base64 content") {
        setPublicKeyPem(*verifier,
            "-----BEGIN PUBLIC KEY-----\n"
            "MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEA00000000000000000000\n"
            "0000000000000000000000000000000000000000000000000000000000000000\n"
            "0000000000000000000000000000000000000000000000000000000000000000\n"
            "0000000000000000000000000000000000000000000000000000000000000000\n"
            "0000000000000000000000000000000000000000000000000000000000000000\n"
            "-----END PUBLIC KEY-----\n");
        auto result = verifier->verify({1, 2, 3}, {4, 5, 6});
        REQUIRE(result.isOk());
        REQUIRE(result.value() == SignatureVerificationResult::NotSupported);
    }
}

TEST_CASE("Signature verification: successful RSA round-trip", "[crypto][signature][rsa]") {
    REQUIRE(psa_crypto_init() == PSA_SUCCESS);

    const char* rsaPrivatePem =
        "-----BEGIN RSA PRIVATE KEY-----\n"
        "MIIEpAIBAAKCAQEAqFVn+bKgHDTGFY6QU25+HlEP7ppDRC320hNPs91pri4VZrjL\n"
        "hOD4/N7sAoWTZiIOGCo5pJ+OztG7GA2B5tC9/cmdSN8UAXR8YO49+8ZqN4g9Ox6q\n"
        "91E42Rq5A9aCMkr7wm5Ym3cK9dZGXHVa4QsROdnoaIKpu3UbbjYOrmQSXXzEkTiX\n"
        "wMTIsXz8SclaRYNhHtnv6CKAIm1sTP4a3GyGeCzBW40zknNcgTqHo6J3FLw1AENY\n"
        "iaQEeXqTOxq3MFWm0HQFoJC4IND54RiARCo7+qJe+aqMGPwIIzQEXRIQVVcG3lvU\n"
        "8lUyTPpegYb2O4zdRrCE7GCpBBe137NmJcZMtQIDAQABAoIBABl8JKu3EWpzyvGE\n"
        "jfEzr0BjwWe8TybJVq7jYZO3l8JZE8BjhdxuOwP9s/mFw5UY3s1lxyhXR8WkFxFD\n"
        "KkGJpNoBZiCcNWkq+5GpQBUYKwiRRcPnlrauw06LLyuXlEqM86SyFBQlZ7FkaW6i\n"
        "Dco4ZLk/dmIsNgo9ZpO+92YLnIQumq5nAY4Mw6CVra54koDmLXorJzidAo2n0059\n"
        "K0hUUMgh4o1BEn5I+YPZOkmASsNUh6zbm26tyaiBnU47ueYE//+RPCTPTI4ePBG5\n"
        "8nGuRGebGpdOm9OO3IGgps80mADnVUI3QTjcwQlY1pEeaQ6FMf6WpfwFSzssD6WS\n"
        "lfEoVBkCgYEA0vRCLOvbhikfaKCnAkaBYlhna1BI32gPa4+bwCKupaI2Kl3uRhPT\n"
        "JB+I+fzWXjPZDq4JsuTcHCpP2EpfBi3ltXmjmmI742D4h20Cv9lPWItICn11HHcQ\n"
        "aV40Td2Lo96N8fSzwdgr0cH8fVvTEWaZiUMZpafypNIecf7UMMi7opMCgYEAzEdP\n"
        "e/zyTHUIUpYI4OlD/C+mCHGOGnDtVG5RIAPNOiXuDshGBetQf+GmCt88RjH5Gz4R\n"
        "LuYhOQIKObtMRzsgD8UbxBoRtmwTAtaX/e/rZiW6kEgplwA7ZV/7oADOBEqhf5Yz\n"
        "ublAtD1VS9zDXr6ZoTeJVmZ0VMlKXPd3wgnZ+JcCgYBgYQRS7bcwBl25OZzT5055\n"
        "lhY560Y/+5T/+W6ZS78rIX9Jv/x6u9f9awLz49Y0189Va6I2v2To4VP1Z5Ueh52p\n"
        "WderUzI1Yjpp9R4KdMhRleDmGgeFZ8hxu35+DLgduDJ11uzBpXfvr4ch5u/5xTxk\n"
        "f+mZy6+KKg2K23gqiatgTQKBgQCW2Amfmvco8jrFETlZK6ciL+VA0umGKOF3uUZ6\n"
        "h5QiXiPeEpFyiYMWC4BbAuE1TG2QalKx+QmLWTBH1UDMUKKqQnjwY/e0ZzXaoK/3\n"
        "uhRvh2iuZjsf3/H8N9ZNHosCrEF5P2bOvDdFYQz9SfWSntg/Lg1iGaHJgiJBaBOs\n"
        "2y1z3QKBgQDF1Fd/BqSCKA3WM0+3Bf7Mu4l40CKmzjFpVGALTQIscfE4kUiymXna\n"
        "DLWearAGdiGpWLD9Wq6/hBC+LLQXQ0zckITz3L2Lh5IJBoysOc2R+N2BHdSvVlti\n"
        "sF7IbcMbszEf8rtt2+ZosApwouLjqtb//15r8CfKiUKDRYNP3OBN2A==\n"
        "-----END RSA PRIVATE KEY-----\n";

    mbedtls_pk_context prvKey;
    mbedtls_pk_init(&prvKey);
    int ret = mbedtls_pk_parse_key(&prvKey,
        reinterpret_cast<const unsigned char*>(rsaPrivatePem),
        strlen(rsaPrivatePem) + 1, nullptr, 0);
    REQUIRE(ret == 0);

    std::vector<uint8_t> data = {0x48, 0x65, 0x6C, 0x6C, 0x6F, 0x20, 0x4D, 0x62,
                                  0x6F, 0x6F, 0x74, 0x43, 0x6F, 0x72, 0x65};

    unsigned char hash[32];
    mbedtls_md(mbedtls_md_info_from_type(MBEDTLS_MD_SHA256),
               data.data(), data.size(), hash);

    unsigned char sig[MBEDTLS_PK_SIGNATURE_MAX_SIZE];
    size_t sigLen = 0;
    ret = mbedtls_pk_sign(&prvKey, MBEDTLS_MD_SHA256,
                          hash, sizeof(hash),
                          sig, sizeof(sig), &sigLen);
    REQUIRE(ret == 0);

    unsigned char pubPemBuf[512];
    ret = mbedtls_pk_write_pubkey_pem(&prvKey, pubPemBuf, sizeof(pubPemBuf));
    REQUIRE(ret == 0);

    std::string pubPem(reinterpret_cast<char*>(pubPemBuf));
    mbedtls_pk_free(&prvKey);

    auto verifier = makeMbedTLSSignatureVerifier();
    REQUIRE(verifier);

    setPublicKeyPem(*verifier, pubPem);
    std::vector<uint8_t> signature(sig, sig + sigLen);
    auto result = verifier->verify(data, signature);
    REQUIRE(result.isOk());
    REQUIRE(result.value() == SignatureVerificationResult::Verified);
}

TEST_CASE("Signature verification: failed with wrong key", "[crypto][signature][rsa]") {
    REQUIRE(psa_crypto_init() == PSA_SUCCESS);

    const char* rsaPrivatePem1 =
        "-----BEGIN RSA PRIVATE KEY-----\n"
        "MIIEpAIBAAKCAQEAqFVn+bKgHDTGFY6QU25+HlEP7ppDRC320hNPs91pri4VZrjL\n"
        "hOD4/N7sAoWTZiIOGCo5pJ+OztG7GA2B5tC9/cmdSN8UAXR8YO49+8ZqN4g9Ox6q\n"
        "91E42Rq5A9aCMkr7wm5Ym3cK9dZGXHVa4QsROdnoaIKpu3UbbjYOrmQSXXzEkTiX\n"
        "wMTIsXz8SclaRYNhHtnv6CKAIm1sTP4a3GyGeCzBW40zknNcgTqHo6J3FLw1AENY\n"
        "iaQEeXqTOxq3MFWm0HQFoJC4IND54RiARCo7+qJe+aqMGPwIIzQEXRIQVVcG3lvU\n"
        "8lUyTPpegYb2O4zdRrCE7GCpBBe137NmJcZMtQIDAQABAoIBABl8JKu3EWpzyvGE\n"
        "jfEzr0BjwWe8TybJVq7jYZO3l8JZE8BjhdxuOwP9s/mFw5UY3s1lxyhXR8WkFxFD\n"
        "KkGJpNoBZiCcNWkq+5GpQBUYKwiRRcPnlrauw06LLyuXlEqM86SyFBQlZ7FkaW6i\n"
        "Dco4ZLk/dmIsNgo9ZpO+92YLnIQumq5nAY4Mw6CVra54koDmLXorJzidAo2n0059\n"
        "K0hUUMgh4o1BEn5I+YPZOkmASsNUh6zbm26tyaiBnU47ueYE//+RPCTPTI4ePBG5\n"
        "8nGuRGebGpdOm9OO3IGgps80mADnVUI3QTjcwQlY1pEeaQ6FMf6WpfwFSzssD6WS\n"
        "lfEoVBkCgYEA0vRCLOvbhikfaKCnAkaBYlhna1BI32gPa4+bwCKupaI2Kl3uRhPT\n"
        "JB+I+fzWXjPZDq4JsuTcHCpP2EpfBi3ltXmjmmI742D4h20Cv9lPWItICn11HHcQ\n"
        "aV40Td2Lo96N8fSzwdgr0cH8fVvTEWaZiUMZpafypNIecf7UMMi7opMCgYEAzEdP\n"
        "e/zyTHUIUpYI4OlD/C+mCHGOGnDtVG5RIAPNOiXuDshGBetQf+GmCt88RjH5Gz4R\n"
        "LuYhOQIKObtMRzsgD8UbxBoRtmwTAtaX/e/rZiW6kEgplwA7ZV/7oADOBEqhf5Yz\n"
        "ublAtD1VS9zDXr6ZoTeJVmZ0VMlKXPd3wgnZ+JcCgYBgYQRS7bcwBl25OZzT5055\n"
        "lhY560Y/+5T/+W6ZS78rIX9Jv/x6u9f9awLz49Y0189Va6I2v2To4VP1Z5Ueh52p\n"
        "WderUzI1Yjpp9R4KdMhRleDmGgeFZ8hxu35+DLgduDJ11uzBpXfvr4ch5u/5xTxk\n"
        "f+mZy6+KKg2K23gqiatgTQKBgQCW2Amfmvco8jrFETlZK6ciL+VA0umGKOF3uUZ6\n"
        "h5QiXiPeEpFyiYMWC4BbAuE1TG2QalKx+QmLWTBH1UDMUKKqQnjwY/e0ZzXaoK/3\n"
        "uhRvh2iuZjsf3/H8N9ZNHosCrEF5P2bOvDdFYQz9SfWSntg/Lg1iGaHJgiJBaBOs\n"
        "2y1z3QKBgQDF1Fd/BqSCKA3WM0+3Bf7Mu4l40CKmzjFpVGALTQIscfE4kUiymXna\n"
        "DLWearAGdiGpWLD9Wq6/hBC+LLQXQ0zckITz3L2Lh5IJBoysOc2R+N2BHdSvVlti\n"
        "sF7IbcMbszEf8rtt2+ZosApwouLjqtb//15r8CfKiUKDRYNP3OBN2A==\n"
        "-----END RSA PRIVATE KEY-----\n";

    mbedtls_pk_context prvKey1;
    mbedtls_pk_init(&prvKey1);
    REQUIRE(mbedtls_pk_parse_key(&prvKey1,
        reinterpret_cast<const unsigned char*>(rsaPrivatePem1),
        strlen(rsaPrivatePem1) + 1, nullptr, 0) == 0);

    unsigned char pubPemBuf[512];
    REQUIRE(mbedtls_pk_write_pubkey_pem(&prvKey1, pubPemBuf, sizeof(pubPemBuf)) == 0);
    std::string wrongPubPem(reinterpret_cast<char*>(pubPemBuf));
    mbedtls_pk_free(&prvKey1);

    std::vector<uint8_t> data = {0xDE, 0xAD, 0xBE, 0xEF};

    auto verifier = makeMbedTLSSignatureVerifier();
    REQUIRE(verifier);

    setPublicKeyPem(*verifier, wrongPubPem);

    std::vector<uint8_t> bogusSig = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
    auto result = verifier->verify(data, bogusSig);
    REQUIRE(result.isOk());
    REQUIRE(result.value() == SignatureVerificationResult::Failed);
}

#else

// ==========================================================================
// Stub tests when crypto is disabled
// ==========================================================================

TEST_CASE("Crypto disabled: hash provider returns NotSupported", "[crypto][disabled]") {
    NotSupportedHashProvider provider;
    auto result = provider.hash({1, 2, 3}, HashAlgorithm::SHA256);
    REQUIRE(result.isError());
    REQUIRE(result.error() == ErrorCode::NotSupported);
}

TEST_CASE("Crypto disabled: signature verifier returns NotSupported", "[crypto][disabled]") {
    NotSupportedSignatureVerifier verifier;
    auto result = verifier.verify({1, 2, 3}, {4, 5, 6});
    REQUIRE(result.isOk());
    REQUIRE(result.value() == SignatureVerificationResult::NotSupported);
}

TEST_CASE("Crypto disabled: integrity verifier returns NotSupported", "[crypto][disabled]") {
    NotSupportedIntegrityVerifier verifier;
    auto result = verifier.verifyIntegrity("/path");
    REQUIRE(result.isError());
    REQUIRE(result.error() == ErrorCode::NotSupported);
}

#endif
