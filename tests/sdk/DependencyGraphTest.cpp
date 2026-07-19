#include <algorithm>
#include <sdk/PluginDependencyGraph.hpp>
#include <sdk/VendorRegistration.hpp>
#include <sdk/ProtocolRegistration.hpp>
#include <sdk/TransportRegistration.hpp>
#include <sdk/WorkflowRegistration.hpp>
#include <sdk/JobRegistration.hpp>
#include <sdk/PackageRegistration.hpp>
#include <sdk/DiscoveryRegistration.hpp>
#include <sdk/CapabilityRegistration.hpp>
#include <catch2/catch_test_macros.hpp>

namespace sdk = mbootcore::sdk;

TEST_CASE("DependencyGraphTest", "[sdk]") {
    SECTION("testEmptyGraph") {
        sdk::PluginDependencyGraph graph;
        REQUIRE(!graph.hasCycle());
        REQUIRE(graph.cycles().empty());

        auto report = graph.resolve();
        REQUIRE(!report.hasCircularDependencies);
        REQUIRE(report.resolvedOrder.empty());
    }

    SECTION("testAddSingleNode") {
        sdk::PluginDependencyGraph graph;
        graph.addNode("node1", "vendor", "VendorA", "1.0.0", {});

        auto report = graph.resolve();
        REQUIRE(report.nodes.size() == 1u);
        REQUIRE(!report.hasCircularDependencies);
        REQUIRE(report.resolvedOrder.size() == 1u);
        REQUIRE(report.resolvedOrder[0] == "node1");
    }

    SECTION("testAddMultipleNodes") {
        sdk::PluginDependencyGraph graph;
        graph.addNode("A", "vendor", "VendorA", "1.0.0", {});
        graph.addNode("B", "protocol", "ProtocolB", "2.0.0", {});
        graph.addNode("C", "transport", "TransportC", "1.5.0", {});

        auto report = graph.resolve();
        REQUIRE(report.nodes.size() == 3u);
        REQUIRE(!report.hasCircularDependencies);
        REQUIRE(report.resolvedOrder.size() == 3u);
    }

    SECTION("testSimpleDependency") {
        sdk::PluginDependencyGraph graph;
        graph.addNode("A", "vendor", "VendorA", "1.0.0", {});
        graph.addNode("B", "protocol", "ProtocolB", "1.0.0", {"A"});
        graph.addNode("C", "transport", "TransportC", "1.0.0", {"B"});

        auto report = graph.resolve();
        REQUIRE(!report.hasCircularDependencies);
        REQUIRE(report.resolvedOrder.size() == 3u);
        REQUIRE(report.resolvedOrder[0] == "A");
        REQUIRE(report.resolvedOrder[1] == "B");
        REQUIRE(report.resolvedOrder[2] == "C");
    }

    SECTION("testCircularDependencyDetection") {
        sdk::PluginDependencyGraph graph;
        graph.addNode("A", "vendor", "VendorA", "1.0.0", {"C"});
        graph.addNode("B", "protocol", "ProtocolB", "1.0.0", {"A"});
        graph.addNode("C", "transport", "TransportC", "1.0.0", {"B"});

        auto report = graph.resolve();
        REQUIRE(report.hasCircularDependencies);
        REQUIRE(!report.cycles.empty());
    }

    SECTION("testNoCircularDependency") {
        sdk::PluginDependencyGraph graph;
        graph.addNode("A", "vendor", "VendorA", "1.0.0", {});
        graph.addNode("B", "protocol", "ProtocolB", "1.0.0", {"A"});
        graph.addNode("C", "transport", "TransportC", "1.0.0", {"B"});
        graph.addNode("D", "job", "JobD", "1.0.0", {"C"});

        auto report = graph.resolve();
        REQUIRE(!report.hasCircularDependencies);
        REQUIRE(report.resolvedOrder.size() == 4u);
    }

    SECTION("testUnresolvedDependency") {
        sdk::PluginDependencyGraph graph;
        graph.addNode("A", "vendor", "VendorA", "1.0.0", {"MissingDep"});

        auto report = graph.resolve();
        REQUIRE(!report.unresolvedDependencies.empty());
        REQUIRE(report.unresolvedDependencies.size() == 1u);
    }

    SECTION("testAddVendorNode") {
        sdk::PluginDependencyGraph graph;
        sdk::VendorRegistration reg;
        reg.name = "GraphVendor";
        reg.version = "1.0.0";
        reg.vendorId = mbootcore::discovery::Vendor::Qualcomm;

        graph.addVendorNode(reg);
        auto report = graph.resolve();
        REQUIRE(report.nodes.size() == 1u);
        REQUIRE(report.nodes.begin()->second.name == "GraphVendor");
    }

    SECTION("testAddProtocolNode") {
        sdk::PluginDependencyGraph graph;
        sdk::ProtocolRegistration reg;
        reg.name = "GraphProtocol";
        reg.version = "2.0.0";
        reg.protocolType = mbootcore::discovery::ProtocolType::Sahara;

        graph.addProtocolNode(reg);
        auto report = graph.resolve();
        REQUIRE(report.nodes.size() == 1u);
        REQUIRE(report.nodes.begin()->second.name == "GraphProtocol");
    }

    SECTION("testAddTransportNode") {
        sdk::PluginDependencyGraph graph;
        sdk::TransportRegistration reg;
        reg.name = "GraphTransport";
        reg.version = "1.5.0";
        reg.transportType = mbootcore::discovery::TransportType::USB;

        graph.addTransportNode(reg);
        auto report = graph.resolve();
        REQUIRE(report.nodes.size() == 1u);
        REQUIRE(report.nodes.begin()->second.name == "GraphTransport");
    }

    SECTION("testAddWorkflowNode") {
        sdk::PluginDependencyGraph graph;
        sdk::WorkflowRegistration reg;
        reg.name = "GraphWorkflow";
        reg.version = "1.0.0";
        reg.isDefault = true;

        graph.addWorkflowNode(reg);
        auto report = graph.resolve();
        REQUIRE(report.nodes.size() == 1u);
        REQUIRE(report.nodes.begin()->second.name == "GraphWorkflow");
    }

    SECTION("testAddJobNode") {
        sdk::PluginDependencyGraph graph;
        sdk::JobRegistration reg;
        reg.name = "GraphJob";
        reg.jobType = "flash";
        reg.version = "1.0.0";

        graph.addJobNode(reg);
        auto report = graph.resolve();
        REQUIRE(report.nodes.size() == 1u);
        REQUIRE(report.nodes.begin()->second.name == "GraphJob");
    }

    SECTION("testAddPackageNode") {
        sdk::PluginDependencyGraph graph;
        sdk::PackageRegistration reg;
        reg.name = "GraphPackage";
        reg.version = "1.0.0";

        graph.addPackageNode(reg);
        auto report = graph.resolve();
        REQUIRE(report.nodes.size() == 1u);
        REQUIRE(report.nodes.begin()->second.name == "GraphPackage");
    }

    SECTION("testAddDiscoveryNode") {
        sdk::PluginDependencyGraph graph;
        sdk::DiscoveryRegistration reg;
        reg.name = "GraphDiscovery";
        reg.version = "1.0.0";

        graph.addDiscoveryNode(reg);
        auto report = graph.resolve();
        REQUIRE(report.nodes.size() == 1u);
        REQUIRE(report.nodes.begin()->second.name == "GraphDiscovery");
    }

    SECTION("testAddCapabilityNode") {
        sdk::PluginDependencyGraph graph;
        sdk::CapabilityRegistration reg;
        reg.name = "GraphCapability";
        reg.version = "1.0.0";
        reg.capability = mbootcore::plugin::PluginCapability::Protocol;

        graph.addCapabilityNode(reg);
        auto report = graph.resolve();
        REQUIRE(report.nodes.size() == 1u);
        REQUIRE(report.nodes.begin()->second.name == "GraphCapability");
    }

    SECTION("testResolveOrder") {
        sdk::PluginDependencyGraph graph;
        graph.addNode("Base", "vendor", "BaseVendor", "1.0.0", {});
        graph.addNode("Mid", "protocol", "MidProtocol", "1.0.0", {"Base"});
        graph.addNode("Top", "transport", "TopTransport", "1.0.0", {"Mid"});

        auto report = graph.resolve();
        REQUIRE(!report.hasCircularDependencies);
        REQUIRE(report.resolvedOrder.size() == 3u);

        auto basePos = std::find(report.resolvedOrder.begin(), report.resolvedOrder.end(), "Base");
        auto midPos = std::find(report.resolvedOrder.begin(), report.resolvedOrder.end(), "Mid");
        auto topPos = std::find(report.resolvedOrder.begin(), report.resolvedOrder.end(), "Top");
        REQUIRE(basePos < midPos);
        REQUIRE(midPos < topPos);
    }

    SECTION("testClearGraph") {
        sdk::PluginDependencyGraph graph;
        graph.addNode("A", "vendor", "VendorA", "1.0.0", {});
        graph.addNode("B", "protocol", "ProtocolB", "1.0.0", {"A"});
        REQUIRE(graph.resolve().nodes.size() == 2u);

        graph.clear();
        REQUIRE(!graph.hasCycle());
        REQUIRE(graph.cycles().empty());

        auto report = graph.resolve();
        REQUIRE(report.nodes.size() == 0u);
    }

    SECTION("testToDotOutput") {
        sdk::PluginDependencyGraph graph;
        graph.addNode("A", "vendor", "VendorA", "1.0.0", {});
        graph.addNode("B", "protocol", "ProtocolB", "1.0.0", {"A"});

        std::string dot = graph.toDot();
        REQUIRE(!dot.empty());
        REQUIRE((dot.find("digraph") != std::string::npos || dot.find("graph") != std::string::npos));
    }

    SECTION("testToJsonOutput") {
        sdk::PluginDependencyGraph graph;
        graph.addNode("A", "vendor", "VendorA", "1.0.0", {});
        graph.addNode("B", "protocol", "ProtocolB", "1.0.0", {"A"});

        std::string json = graph.toJson();
        REQUIRE(!json.empty());
        REQUIRE(json.find("A") != std::string::npos);
        REQUIRE(json.find("B") != std::string::npos);
    }

    SECTION("testToPlantUmlOutput") {
        sdk::PluginDependencyGraph graph;
        graph.addNode("A", "vendor", "VendorA", "1.0.0", {});
        graph.addNode("B", "protocol", "ProtocolB", "1.0.0", {"A"});

        std::string uml = graph.toPlantUml();
        REQUIRE(!uml.empty());
        REQUIRE((uml.find("@start") != std::string::npos || uml.find("-->") != std::string::npos));
    }
}
