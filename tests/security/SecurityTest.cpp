#include <catch2/catch_test_macros.hpp>
#include <memory>
#include <string>
#include <vector>
#include <chrono>

#include <mbootcore/security/SecurityTypes.hpp>
#include <mbootcore/security/SecurityManager.hpp>
#include <mbootcore/security/PermissionSet.hpp>
#include <mbootcore/security/InMemoryStorage.hpp>
#include <mbootcore/security/interfaces/IHashProvider.hpp>
#include <mbootcore/security/interfaces/ISignatureVerifier.hpp>
#include <mbootcore/security/interfaces/ISecureStorage.hpp>
#include <mbootcore/security/interfaces/IIntegrityVerifier.hpp>
#include <mbootcore/domain/Error.hpp>

using namespace mbootcore;
using namespace mbootcore::security;

TEST_CASE("SecurityTypes", "[security]") {
    SECTION("HashAlgorithmEnum") {
        REQUIRE(static_cast<uint32_t>(HashAlgorithm::SHA256) == 0u);
        REQUIRE(static_cast<uint32_t>(HashAlgorithm::SHA512) == 1u);
    }
    SECTION("SignatureAlgorithmEnum") {
        REQUIRE(static_cast<uint32_t>(SignatureAlgorithm::RSA2048) == 0u);
        REQUIRE(static_cast<uint32_t>(SignatureAlgorithm::RSA4096) == 1u);
        REQUIRE(static_cast<uint32_t>(SignatureAlgorithm::None) == 2u);
    }
    SECTION("SecurityLevelEnum") {
        REQUIRE(static_cast<uint32_t>(SecurityLevel::None) == 0u);
        REQUIRE(static_cast<uint32_t>(SecurityLevel::Basic) == 1u);
        REQUIRE(static_cast<uint32_t>(SecurityLevel::Standard) == 2u);
        REQUIRE(static_cast<uint32_t>(SecurityLevel::High) == 3u);
        REQUIRE(static_cast<uint32_t>(SecurityLevel::FIPS) == 4u);
    }
    SECTION("VerificationResultEnum") {
        REQUIRE(static_cast<uint32_t>(VerificationResult::Verified) == 0u);
        REQUIRE(static_cast<uint32_t>(VerificationResult::Failed) == 1u);
        REQUIRE(static_cast<uint32_t>(VerificationResult::NotSigned) == 2u);
        REQUIRE(static_cast<uint32_t>(VerificationResult::Unknown) == 3u);
        REQUIRE(static_cast<uint32_t>(VerificationResult::Tampered) == 4u);
    }
    SECTION("SecurityPolicyDefaults") {
        SecurityPolicy policy;
        REQUIRE(policy.level == SecurityLevel::None);
        REQUIRE(policy.requirePackageSignatures == false);
        REQUIRE(policy.requirePluginSignatures == false);
        REQUIRE(policy.requireDSPSignatures == false);
        REQUIRE(policy.tamperDetection == false);
        REQUIRE(policy.hashAlgorithm == HashAlgorithm::SHA256);
        REQUIRE(policy.signatureAlgorithm == SignatureAlgorithm::None);
        REQUIRE(policy.trustedKeys.empty());
    }
    SECTION("SignatureInfoDefaults") {
        SignatureInfo info;
        REQUIRE(info.packageId.empty());
        REQUIRE(info.signer.empty());
        REQUIRE(info.hashValue.empty());
        REQUIRE(info.algorithm == SignatureAlgorithm::None);
        REQUIRE(info.valid == false);
    }
    SECTION("SecureHashDefaults") {
        SecureHash hash;
        REQUIRE(hash.value.empty());
        REQUIRE(hash.algorithm == HashAlgorithm::SHA256);
        REQUIRE(hash.toString() == std::string("(empty)"));
    }
    SECTION("SecureHashComparison") {
        SecureHash a;
        a.value = "abc123";
        a.algorithm = HashAlgorithm::SHA256;

        SecureHash b;
        b.value = "abc123";
        b.algorithm = HashAlgorithm::SHA256;

        SecureHash c;
        c.value = "def456";
        c.algorithm = HashAlgorithm::SHA256;

        REQUIRE(a == b);
        REQUIRE(a != c);
    }
}

TEST_CASE("SecurityManager", "[security]") {
    SECTION("PolicySetGet") {
        SecurityManager mgr;
        SecurityPolicy policy;
        policy.level = SecurityLevel::High;

        auto setResult = mgr.setPolicy(policy);
        REQUIRE(setResult.isOk());

        auto getResult = mgr.policy();
        REQUIRE(getResult.isOk());
        REQUIRE(getResult.value().level == SecurityLevel::High);
        REQUIRE(getResult.value().requirePackageSignatures == false);
        REQUIRE(mgr.securityLevel() == SecurityLevel::High);
    }

    SECTION("VerifyPackageSignatureReturnsNotSupported") {
        SecurityManager mgr;
        auto result = mgr.verifyPackageSignature("any_file.bin");
        REQUIRE(result.isError());
        REQUIRE(result.error() == ErrorCode::NotSupported);
    }

    SECTION("VerifyPluginSignatureReturnsNotSupported") {
        SecurityManager mgr;
        auto result = mgr.verifyPluginSignature("any_plugin.so");
        REQUIRE(result.isError());
        REQUIRE(result.error() == ErrorCode::NotSupported);
    }

    SECTION("VerifyDSPSignatureReturnsNotSupported") {
        SecurityManager mgr;
        auto result = mgr.verifyDSPSignature("any_dsp.bin");
        REQUIRE(result.isError());
        REQUIRE(result.error() == ErrorCode::NotSupported);
    }

    SECTION("ComputeHashReturnsNotSupported") {
        SecurityManager mgr;
        auto result = mgr.computeHash("data", HashAlgorithm::SHA256);
        REQUIRE(result.isError());
        REQUIRE(result.error() == ErrorCode::NotSupported);
    }

    SECTION("ComputeHashSHA512ReturnsNotSupported") {
        SecurityManager mgr;
        auto result = mgr.computeHash("data", HashAlgorithm::SHA512);
        REQUIRE(result.isError());
        REQUIRE(result.error() == ErrorCode::NotSupported);
    }

    SECTION("IsTamperedReturnsNotSupported") {
        SecurityManager mgr;
        auto result = mgr.isTampered("any/path");
        REQUIRE(result.isError());
        REQUIRE(result.error() == ErrorCode::NotSupported);
    }

    SECTION("TrustedKeysManagement") {
        SecurityManager mgr;

        auto keys = mgr.trustedKeys();
        REQUIRE(keys.isOk());
        REQUIRE(keys.value().empty());

        auto add1 = mgr.addTrustedKey("key1");
        REQUIRE(add1.isOk());

        auto add2 = mgr.addTrustedKey("key2");
        REQUIRE(add2.isOk());

        auto addDup = mgr.addTrustedKey("key1");
        REQUIRE(addDup.isOk());

        keys = mgr.trustedKeys();
        REQUIRE(keys.isOk());
        REQUIRE(keys.value().size() == 2u);

        auto rm = mgr.removeTrustedKey("key1");
        REQUIRE(rm.isOk());

        keys = mgr.trustedKeys();
        REQUIRE(keys.isOk());
        REQUIRE(keys.value().size() == 1u);
        REQUIRE(keys.value()[0] == std::string("key2"));
    }
}

TEST_CASE("PermissionSet", "[security]") {
    SECTION("DefaultHasNoPermissions") {
        PermissionSet ps;
        REQUIRE(ps.count() == 0u);
        REQUIRE(!ps.has(Permission::Read));
        REQUIRE(!ps.has(Permission::Write));
        REQUIRE(!ps.has(Permission::Execute));
        REQUIRE(!ps.has(Permission::Admin));
    }
    SECTION("GrantAndHas") {
        PermissionSet ps;
        ps.grant(Permission::Read);
        ps.grant(Permission::Write);
        REQUIRE(ps.has(Permission::Read));
        REQUIRE(ps.has(Permission::Write));
        REQUIRE(!ps.has(Permission::Execute));
        REQUIRE(ps.count() == 2u);
    }
    SECTION("Revoke") {
        PermissionSet ps;
        ps.grant(Permission::Read);
        ps.grant(Permission::Write);
        REQUIRE(ps.has(Permission::Read));
        ps.revoke(Permission::Read);
        REQUIRE(!ps.has(Permission::Read));
        REQUIRE(ps.has(Permission::Write));
        REQUIRE(ps.count() == 1u);
    }
    SECTION("HasAll") {
        PermissionSet ps;
        ps.grant(Permission::Read);
        ps.grant(Permission::Write);
        ps.grant(Permission::Execute);
        REQUIRE(ps.hasAll({Permission::Read, Permission::Write}));
        REQUIRE(ps.hasAll({Permission::Read, Permission::Write, Permission::Execute}));
        REQUIRE(!ps.hasAll({Permission::Read, Permission::Admin}));
    }
    SECTION("HasAny") {
        PermissionSet ps;
        ps.grant(Permission::Read);
        REQUIRE(ps.hasAny({Permission::Read, Permission::Write}));
        REQUIRE(ps.hasAny({Permission::Read}));
        REQUIRE(!ps.hasAny({Permission::Write, Permission::Execute}));
    }
    SECTION("Clear") {
        PermissionSet ps;
        ps.grant(Permission::Read);
        ps.grant(Permission::Admin);
        REQUIRE(ps.count() == 2u);
        ps.clear();
        REQUIRE(ps.count() == 0u);
        REQUIRE(!ps.has(Permission::Read));
        REQUIRE(!ps.has(Permission::Admin));
    }
    SECTION("ConstructionFromVector") {
        std::vector<Permission> perms = {Permission::Read, Permission::Write, Permission::Execute};
        PermissionSet ps(perms);
        REQUIRE(ps.has(Permission::Read));
        REQUIRE(ps.has(Permission::Write));
        REQUIRE(ps.has(Permission::Execute));
        REQUIRE(!ps.has(Permission::Admin));
        REQUIRE(ps.count() == 3u);
    }
    SECTION("Equality") {
        PermissionSet a;
        PermissionSet b;
        REQUIRE(a == b);

        a.grant(Permission::Read);
        REQUIRE(a != b);

        b.grant(Permission::Read);
        REQUIRE(a == b);
    }
}

TEST_CASE("InMemoryStorage", "[security][testing]") {
    using namespace mbootcore::security::testing;

    SECTION("StoreAndRetrieve") {
        InMemoryStorage storage;
        auto storeResult = storage.store("key1", "value1");
        REQUIRE(storeResult.isOk());

        auto retrieveResult = storage.retrieve("key1");
        REQUIRE(retrieveResult.isOk());
        REQUIRE(retrieveResult.value() == std::string("value1"));
    }
    SECTION("NonExistentKey") {
        InMemoryStorage storage;
        auto result = storage.retrieve("nonexistent");
        REQUIRE(result.isError());
    }
    SECTION("Overwrite") {
        InMemoryStorage storage;
        REQUIRE(storage.store("key1", "value1").isOk());
        REQUIRE(storage.store("key1", "value2").isOk());

        auto result = storage.retrieve("key1");
        REQUIRE(result.isOk());
        REQUIRE(result.value() == std::string("value2"));
    }
    SECTION("Remove") {
        InMemoryStorage storage;
        REQUIRE(storage.store("key1", "value1").isOk());
        REQUIRE(storage.exists("key1").value());

        REQUIRE(storage.remove("key1").isOk());
        REQUIRE(!storage.exists("key1").value());
    }
    SECTION("Exists") {
        InMemoryStorage storage;
        REQUIRE(!storage.exists("key1").value());

        REQUIRE(storage.store("key1", "value1").isOk());
        REQUIRE(storage.exists("key1").value());
    }
    SECTION("ClearStorage") {
        InMemoryStorage storage;
        REQUIRE(storage.store("key1", "value1").isOk());
        REQUIRE(storage.store("key2", "value2").isOk());
        REQUIRE(storage.exists("key1").value());

        REQUIRE(storage.clear().isOk());
        REQUIRE(!storage.exists("key1").value());
        REQUIRE(!storage.exists("key2").value());
    }
}

TEST_CASE("IHashProvider", "[security][interfaces]") {
    SECTION("NotSupportedHashProvider") {
        NotSupportedHashProvider provider;
        std::vector<uint8_t> data = {1, 2, 3};
        auto result = provider.hash(data, HashAlgorithm::SHA256);
        REQUIRE(result.isError());
        REQUIRE(result.error() == ErrorCode::NotSupported);
    }
}

TEST_CASE("ISignatureVerifier", "[security][interfaces]") {
    SECTION("NotSupportedSignatureVerifier") {
        NotSupportedSignatureVerifier verifier;
        std::vector<uint8_t> data = {1, 2, 3};
        std::vector<uint8_t> sig = {4, 5, 6};
        auto result = verifier.verify(data, sig);
        REQUIRE(result.isOk());
        REQUIRE(result.value() == SignatureVerificationResult::NotSupported);
    }
    SECTION("NotSupportedSignatureVerifierFile") {
        NotSupportedSignatureVerifier verifier;
        auto result = verifier.verifyFile("/path/to/file");
        REQUIRE(result.isOk());
        REQUIRE(result.value() == SignatureVerificationResult::NotSupported);
    }
}

TEST_CASE("ISecureStorage", "[security][interfaces]") {
    SECTION("NotSupportedSecureStorage") {
        NotSupportedSecureStorage storage;
        REQUIRE(storage.store("k", "v").isError());
        REQUIRE(storage.retrieve("k").isError());
        REQUIRE(storage.remove("k").isError());
        REQUIRE(storage.exists("k").isError());
        REQUIRE(storage.clear().isError());
    }
}

TEST_CASE("IIntegrityVerifier", "[security][interfaces]") {
    SECTION("NotSupportedIntegrityVerifier") {
        NotSupportedIntegrityVerifier verifier;
        REQUIRE(verifier.isTampered("/path").isError());
        REQUIRE(verifier.verifyIntegrity("/path").isError());
    }
}
