#include <catch2/catch_test_macros.hpp>
#include <mbootcore/vendor/VendorTypes.hpp>
#include <mbootcore/vendor/VendorRegistry.hpp>
#include <mbootcore/vendor/IVendor.hpp>
#include <mbootcore/vendor/IVendorPlugin.hpp>
#include <mbootcore/domain/Error.hpp>
#include <mbootcore/discovery/IDeviceDetector.hpp>
#include <mbootcore/discovery/IProtocolNegotiator.hpp>
#include <mbootcore/pipeline/BootPipeline.hpp>
#include <mbootcore/generic/IFlashDevice.hpp>
#include <mbootcore/workflow/WorkflowEngine.hpp>
#include <memory>
#include <string>
#include <vector>

using namespace mbootcore;
using namespace mbootcore::vendor;

namespace {

class TestVendor : public IVendor {
public:
    TestVendor(std::string id, std::string name, VendorFamily family,
               VendorMaturity mat = VendorMaturity::Experimental)
        : m_id(std::move(id)), m_name(std::move(name)), m_family(family), m_maturity(mat) {}

    Result<void> initialize(const VendorContext&) override { return {}; }
    Result<void> shutdown() noexcept override { return {}; }

    VendorInfo vendorInfo() const override {
        VendorInfo info;
        info.id = m_id;
        info.name = m_name;
        info.family = m_family;
        info.maturity = m_maturity;
        return info;
    }

    VendorCapability capabilities() const override { return VendorCapability::BootROM; }
    std::unique_ptr<discovery::IDeviceDetector> createDetector() override { return nullptr; }
    std::unique_ptr<discovery::IProtocolNegotiator> createNegotiator() override { return nullptr; }
    std::unique_ptr<pipeline::BootPipeline> createPipeline() override { return nullptr; }
    std::unique_ptr<IFlashDevice> createFlashDevice() override { return nullptr; }
    std::unique_ptr<workflow::WorkflowEngine> createWorkflow() override { return nullptr; }
    std::string_view name() const noexcept override { return m_name; }
    std::unique_ptr<IVendor> clone() const override {
        return std::make_unique<TestVendor>(m_id, m_name, m_family, m_maturity);
    }

private:
    std::string m_id;
    std::string m_name;
    VendorFamily m_family;
    VendorMaturity m_maturity;
};

} // anonymous namespace

TEST_CASE("VendorMaturityModel", "[vendor][maturity]") {
    SECTION("maturityEnumValues") {
        REQUIRE(static_cast<uint32_t>(VendorMaturity::Experimental) == 0);
        REQUIRE(static_cast<uint32_t>(VendorMaturity::Scaffold) == 1);
        REQUIRE(static_cast<uint32_t>(VendorMaturity::Preview) == 2);
        REQUIRE(static_cast<uint32_t>(VendorMaturity::Production) == 3);
    }

    SECTION("defaultMaturityIsExperimental") {
        VendorInfo info;
        info.id = "test";
        info.name = "Test";
        REQUIRE(info.maturity == VendorMaturity::Experimental);
    }

    SECTION("vendorInfoReportsMaturity") {
        TestVendor v("test", "Test", VendorFamily::Custom, VendorMaturity::Scaffold);
        auto info = v.vendorInfo();
        REQUIRE(info.maturity == VendorMaturity::Scaffold);
    }

    SECTION("productionVendorReportsCorrectMaturity") {
        TestVendor v("qcom", "Qualcomm", VendorFamily::Qualcomm, VendorMaturity::Production);
        auto info = v.vendorInfo();
        REQUIRE(info.maturity == VendorMaturity::Production);
        REQUIRE(info.family == VendorFamily::Qualcomm);
    }

    SECTION("maturityProgression") {
        REQUIRE(static_cast<uint32_t>(VendorMaturity::Experimental) <
                static_cast<uint32_t>(VendorMaturity::Scaffold));
        REQUIRE(static_cast<uint32_t>(VendorMaturity::Scaffold) <
                static_cast<uint32_t>(VendorMaturity::Preview));
        REQUIRE(static_cast<uint32_t>(VendorMaturity::Preview) <
                static_cast<uint32_t>(VendorMaturity::Production));
    }

    SECTION("registryReportsMaturity") {
        VendorRegistry reg;
        auto vendor = std::make_unique<TestVendor>(
            "reg_test", "RegTest", VendorFamily::Custom, VendorMaturity::Preview);
        auto result = reg.registerVendor(std::move(vendor));
        REQUIRE(result.isOk());

        auto* resolved = reg.resolveById("reg_test");
        REQUIRE(resolved != nullptr);
        REQUIRE(resolved->vendorInfo().maturity == VendorMaturity::Preview);
    }

    SECTION("vendorClonePreservesMaturity") {
        TestVendor original("clone_test", "CloneTest", VendorFamily::Custom, VendorMaturity::Scaffold);
        auto cloned = original.clone();
        REQUIRE(cloned->vendorInfo().maturity == VendorMaturity::Scaffold);
    }

    SECTION("multipleVendorsWithDifferentMaturity") {
        VendorRegistry reg;
        (void)reg.registerVendor(std::make_unique<TestVendor>(
            "v1", "V1", VendorFamily::Qualcomm, VendorMaturity::Production));
        (void)reg.registerVendor(std::make_unique<TestVendor>(
            "v2", "V2", VendorFamily::MediaTek, VendorMaturity::Scaffold));
        (void)reg.registerVendor(std::make_unique<TestVendor>(
            "v3", "V3", VendorFamily::UNISOC, VendorMaturity::Scaffold));

        size_t productionCount = 0;
        size_t scaffoldCount = 0;
        for (auto* v : reg.allVendors()) {
            if (v) {
                auto mat = v->vendorInfo().maturity;
                if (mat == VendorMaturity::Production) ++productionCount;
                else if (mat == VendorMaturity::Scaffold) ++scaffoldCount;
            }
        }
        REQUIRE(productionCount == 1);
        REQUIRE(scaffoldCount == 2);
    }
}
