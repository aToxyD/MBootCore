#pragma once

#include <mbootcore/firmware/FirmwareTypes.hpp>
#include <mbootcore/firmware/FirmwarePackage.hpp>
#include <string>

namespace mbootcore {
namespace firmware {

class ImageEngine {
public:
    ImageEngine() = default;

    Result<FirmwareImage> loadImageData(FirmwareImage image,
                                         const std::string& sourcePath);
    Result<void> verifyImage(const FirmwareImage& image) const;
    Result<void> verifyAllImages(const FirmwarePackage& pkg) const;
    
    Result<ByteBuffer> extractRaw(const FirmwareImage& image) const;
    Result<void> extractToFile(const FirmwareImage& image,
                                const std::string& outputPath) const;
    
    Result<ByteBuffer> convertToRaw(const FirmwareImage& image) const;
};

} // namespace firmware
} // namespace mbootcore
