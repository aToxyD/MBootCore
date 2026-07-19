#pragma once

#include <catch2/catch_test_macros.hpp>
#include <mbootcore/diagnostics/DiagnosticCheck.hpp>
#include <mbootcore/vendor/VendorTypes.hpp>
#include <mbootcore/vendor/VendorRegistry.hpp>
#include <mbootcore/vendor/VendorFactory.hpp>
#include <string>
#include <vector>

namespace mbootcore {
namespace diagnostics {

class VendorMaturityCheck : public DiagnosticCheck {
public:
    VendorMaturityCheck() = default;

    std::string id() const override { return "check-vendor-maturity"; }
    std::string name() const override { return "Vendor Maturity Check"; }
    DiagnosticCategory category() const override { return DiagnosticCategory::Runtime; }

    DiagnosticIssue execute() override {
        DiagnosticIssue issue;
        issue.id = id();
        issue.title = name();
        issue.severity = DiagnosticSeverity::Info;
        issue.category = category();
        issue.status = "Completed";

        auto& reg = vendor::VendorFactory::registry();
        auto vendors = reg.allVendors();

        issue.details["totalVendors"] = std::to_string(vendors.size());

        size_t productionCount = 0;
        size_t scaffoldCount = 0;
        size_t previewCount = 0;
        size_t experimentalCount = 0;

        for (auto* v : vendors) {
            if (v) {
                auto maturity = v->vendorInfo().maturity;
                switch (maturity) {
                    case vendor::VendorMaturity::Production:
                        ++productionCount;
                        break;
                    case vendor::VendorMaturity::Scaffold:
                        ++scaffoldCount;
                        break;
                    case vendor::VendorMaturity::Preview:
                        ++previewCount;
                        break;
                    case vendor::VendorMaturity::Experimental:
                        ++experimentalCount;
                        break;
                }
            }
        }

        issue.details["productionVendors"] = std::to_string(productionCount);
        issue.details["scaffoldVendors"] = std::to_string(scaffoldCount);
        issue.details["previewVendors"] = std::to_string(previewCount);
        issue.details["experimentalVendors"] = std::to_string(experimentalCount);

        std::string summary;
        for (auto* v : vendors) {
            if (v) {
                auto info = v->vendorInfo();
                if (!summary.empty()) summary += ", ";
                summary += info.name + "(" + vendorMaturityString(info.maturity) + ")";
            }
        }
        issue.details["vendorSummary"] = summary;

        return issue;
    }

private:
    static std::string vendorMaturityString(vendor::VendorMaturity m) {
        switch (m) {
            case vendor::VendorMaturity::Experimental: return "experimental";
            case vendor::VendorMaturity::Scaffold: return "scaffold";
            case vendor::VendorMaturity::Preview: return "preview";
            case vendor::VendorMaturity::Production: return "production";
        }
        return "unknown";
    }
};

} // namespace diagnostics
} // namespace mbootcore
