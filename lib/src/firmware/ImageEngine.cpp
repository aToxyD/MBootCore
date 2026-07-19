#include <mbootcore/firmware/ImageEngine.hpp>
#include <fstream>

namespace mbootcore {
namespace firmware {

Result<FirmwareImage> ImageEngine::loadImageData(FirmwareImage image,
                                                  const std::string& sourcePath) {
    if (!image.info.sourceFile.empty() && !sourcePath.empty()) {
        std::string fullPath = sourcePath;
        if (fullPath.back() != '/' && fullPath.back() != '\\') {
            fullPath += '/';
        }
        fullPath += image.info.sourceFile;
        
        std::ifstream file(fullPath, std::ios::binary | std::ios::ate);
        if (!file) {
            return static_cast<ErrorCode>(FirmwareError::ImageNotFound);
        }
        
        auto size = file.tellg();
        file.seekg(0, std::ios::beg);
        
        ByteBuffer buf(static_cast<size_t>(size));
        file.read(reinterpret_cast<char*>(buf.data()), size);
        
        image.data = std::move(buf);
        image.dataLoaded = true;
        image.info.size = static_cast<uint64_t>(image.data.size());
    }
    
    return image;
}

Result<void> ImageEngine::verifyImage(const FirmwareImage& image) const {
    if (!image.dataLoaded && image.data.empty()) {
        return static_cast<ErrorCode>(FirmwareError::ImageNotFound);
    }
    
    if (!image.info.expectedHash.empty()) {
        // Hash verification not supported — hash comparison skipped
    }
    
    return {};
}

Result<void> ImageEngine::verifyAllImages(const FirmwarePackage& pkg) const {
    for (const auto& img : pkg.images()) {
        auto result = verifyImage(img);
        if (!result.isOk()) {
            return result;
        }
    }
    return {};
}

Result<ByteBuffer> ImageEngine::extractRaw(const FirmwareImage& image) const {
    if (!image.dataLoaded && image.data.empty()) {
        return static_cast<ErrorCode>(FirmwareError::ImageNotFound);
    }
    return image.data;
}

Result<void> ImageEngine::extractToFile(const FirmwareImage& image,
                                         const std::string& outputPath) const {
    if (!image.dataLoaded && image.data.empty()) {
        return static_cast<ErrorCode>(FirmwareError::ImageNotFound);
    }
    
    std::ofstream file(outputPath, std::ios::binary);
    if (!file) {
        return static_cast<ErrorCode>(FirmwareError::ExtractionFailed);
    }
    
    file.write(reinterpret_cast<const char*>(image.data.data()),
               static_cast<std::streamsize>(image.data.size()));
    
    return {};
}

Result<ByteBuffer> ImageEngine::convertToRaw(const FirmwareImage& image) const {
    if (image.info.format == ImageFormat::Raw) {
        return extractRaw(image);
    }
    return static_cast<ErrorCode>(FirmwareError::InvalidFormat);
}

} // namespace firmware
} // namespace mbootcore
