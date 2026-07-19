#pragma once

#include <memory>
#include <string>
#include <vector>

#include <mbootcore/dsp/DSPTypes.hpp>
#include <mbootcore/dsp/DSPMetadata.hpp>

namespace mbootcore {
namespace dsp {

struct BuildConfig {
    std::string sourceDir;
    std::string outputDir;
    std::string packageId;
    std::string name;
    std::string vendor;
    std::string version;
    std::string description;
    std::vector<std::string> authors;
    std::string license;
    bool compressLoaders{false};
    bool generateChecksum{true};
    bool generateManifest{true};
    bool generateDependencyGraph{true};
    bool generateChecksumManifest{false};
    std::string outputFileName;
    std::vector<std::string> includePatterns;
    std::vector<std::string> excludePatterns;
};

class DSPBuilder {
public:
    DSPBuilder();
    ~DSPBuilder();

    Result<void> build(const BuildConfig& config);
    Result<void> buildFromManifest(const std::string& manifestPath, const std::string& outputDir);

    Result<void> generateManifest(const BuildConfig& config, const std::string& outputPath);
    Result<void> generateChecksums(const std::string& packagePath);
    Result<void> generateDependencies(const std::string& packagePath);
    Result<void> compressLoaders(const std::string& packagePath);

    /// Generate a checksum manifest (signature.sig) for a built package.
    /// The manifest contains SHA-256 digests for integrity verification only.
    /// It is NOT a cryptographic signature. No private key is used.
    Result<void> generateChecksumManifest(const std::string& packagePath);

    Result<void> validateBuiltPackage(const std::string& packagePath);

    const BuildConfig& config() const noexcept;
    std::vector<std::string> generatedFiles() const noexcept;
    std::vector<std::string> warnings() const noexcept;

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace dsp
} // namespace mbootcore
