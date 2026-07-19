#include <mbootcore/firmware/FirmwareReaders.hpp>
#include <mbootcore/firmware/FirmwarePackage.hpp>

#include "SafeParser.hpp"

#include <fstream>
#include <sstream>
#include <filesystem>
#include <cctype>
#include <algorithm>

namespace fs = std::filesystem;

namespace mbootcore {
namespace firmware {

// ============================================================
// Helper: simple JSON-like manifest parser
// ============================================================

namespace {

std::string extractValue(const std::string& line, const std::string& key) {
    auto pos = line.find('"' + key + '"');
    if (pos == std::string::npos) return {};
    auto colon = line.find(':', pos);
    if (colon == std::string::npos) return {};
    auto valStart = line.find_first_not_of(" \t\":", colon + 1);
    if (valStart == std::string::npos) return {};
    auto valEnd = line.find_first_of(",\n\r}", valStart);
    if (valEnd == std::string::npos) valEnd = line.size();
    auto val = line.substr(valStart, valEnd - valStart);
    // Trim quotes and whitespace
    val.erase(std::remove(val.begin(), val.end(), '"'), val.end());
    val.erase(std::remove(val.begin(), val.end(), ','), val.end());
    auto f = std::find_if_not(val.begin(), val.end(), [](unsigned char c){ return std::isspace(c); });
    val.erase(val.begin(), f);
    std::reverse(val.begin(), val.end());
    f = std::find_if_not(val.begin(), val.end(), [](unsigned char c){ return std::isspace(c); });
    val.erase(val.begin(), f);
    std::reverse(val.begin(), val.end());
    return val;
}

FirmwareVersion parseVersion(const std::string& ver) {
    FirmwareVersion v;
    auto p1 = ver.find('.');
    if (p1 != std::string::npos) {
        auto r1 = fromCharsUint32(ver.substr(0, p1));
        if (r1.ok) v.major = r1.value;
        auto p2 = ver.find('.', p1 + 1);
        if (p2 != std::string::npos) {
            auto r2 = fromCharsUint32(ver.substr(p1 + 1, p2 - p1 - 1));
            auto r3 = fromCharsUint32(ver.substr(p2 + 1));
            if (r2.ok) v.minor = r2.value;
            if (r3.ok) v.patch = r3.value;
        } else {
            auto r2 = fromCharsUint32(ver.substr(p1 + 1));
            if (r2.ok) v.minor = r2.value;
        }
    } else if (!ver.empty()) {
        auto r = fromCharsUint32(ver);
        if (r.ok) v.major = r.value;
    }
    return v;
}

ImageType parseImageType(const std::string& type) {
    if (type == "Programmer") return ImageType::Programmer;
    if (type == "Bootloader") return ImageType::Bootloader;
    if (type == "GPT") return ImageType::GPT;
    if (type == "Boot") return ImageType::Boot;
    if (type == "System") return ImageType::System;
    if (type == "Vendor") return ImageType::Vendor;
    if (type == "Userdata") return ImageType::Userdata;
    if (type == "Recovery") return ImageType::Recovery;
    if (type == "Cache") return ImageType::Cache;
    return ImageType::Custom;
}

ByteBuffer readFile(const std::string& path) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file) return {};
    auto size = file.tellg();
    file.seekg(0, std::ios::beg);
    ByteBuffer buf(static_cast<size_t>(size));
    file.read(reinterpret_cast<char*>(buf.data()), size);
    return buf;
}

} // anonymous namespace

// ============================================================
// DirectoryFirmwareReader
// ============================================================

Result<std::unique_ptr<FirmwarePackage>> DirectoryFirmwareReader::read(const std::string& path) {
    fs::path dir(path);
    if (!fs::is_directory(dir)) {
        return static_cast<ErrorCode>(FirmwareError::InvalidFormat);
    }

    auto manifestPath = dir / "manifest.json";
    if (!fs::exists(manifestPath)) {
        return static_cast<ErrorCode>(FirmwareError::InvalidManifest);
    }

    std::ifstream mf(manifestPath);
    if (!mf) {
        return static_cast<ErrorCode>(FirmwareError::InvalidManifest);
    }

    std::stringstream ss;
    ss << mf.rdbuf();
    std::string content = ss.str();

    PackageMetadata meta;
    meta.vendor = extractValue(content, "vendor");
    meta.platform = extractValue(content, "platform");
    meta.chipset = extractValue(content, "chipset");
    meta.protocol = extractValue(content, "protocol");
    meta.author = extractValue(content, "author");
    meta.buildDate = extractValue(content, "build_date");
    std::string verStr = extractValue(content, "version");
    if (!verStr.empty()) {
        meta.version = parseVersion(verStr);
    }

    auto pkg = std::make_unique<FirmwarePackage>(std::move(meta));

    PackageManifest manifest;

    // Parse images section
    auto imagesPos = content.find("\"images\"");
    std::vector<FirmwareImageInfo> imageInfos;
    if (imagesPos != std::string::npos) {
        auto bracePos = content.find('[', imagesPos);
        if (bracePos != std::string::npos) {
            auto closeBrace = content.find(']', bracePos);
            auto imagesContent = content.substr(bracePos + 1, closeBrace - bracePos - 1);
            
            std::string::size_type start = 0;
            while (true) {
                auto objStart = imagesContent.find('{', start);
                if (objStart == std::string::npos) break;
                auto objEnd = imagesContent.find('}', objStart);
                if (objEnd == std::string::npos) break;
                auto obj = imagesContent.substr(objStart, objEnd - objStart + 1);
                
                FirmwareImageInfo imgInfo;
                std::string name = extractValue(obj, "name");
                if (!name.empty()) {
                    imgInfo.name = name;
                    imgInfo.partitionName = extractValue(obj, "partition");
                    if (imgInfo.partitionName.empty()) imgInfo.partitionName = name;
                    imgInfo.sourceFile = extractValue(obj, "file");
                    if (imgInfo.sourceFile.empty()) imgInfo.sourceFile = name + ".bin";
                    std::string typeStr = extractValue(obj, "type");
                    if (!typeStr.empty()) imgInfo.type = parseImageType(typeStr);
                    std::string offStr = extractValue(obj, "offset");
                    if (!offStr.empty()) {
                        auto r = fromCharsUint64(offStr);
                        if (r.ok) imgInfo.offset = r.value;
                    }
                    std::string sizeStr = extractValue(obj, "size");
                    if (!sizeStr.empty()) {
                        auto r = fromCharsUint64(sizeStr);
                        if (r.ok) imgInfo.size = r.value;
                    }
                    
                    // Load actual image data
                    auto filePath = dir / imgInfo.sourceFile;
                    auto data = readFile(filePath.string());
                    if (!data.empty()) {
                        FirmwareImage image;
                        image.info = imgInfo;
                        image.info.size = data.size();
                        image.data = std::move(data);
                        image.dataLoaded = true;
                        (void)pkg->addImage(std::move(image));
                        imageInfos.push_back(imgInfo);
                    }
                }
                
                start = objEnd + 1;
            }
        }
    }

    manifest.images = std::move(imageInfos);
    pkg->setManifest(std::move(manifest));
    
    return pkg;
}

bool DirectoryFirmwareReader::canRead(const std::string& path) const {
    fs::path p(path);
    return fs::is_directory(p);
}

// ============================================================
// RawFirmwareReader
// ============================================================

RawFirmwareReader::RawFirmwareReader(const std::string& partitionName)
    : m_partitionName(partitionName) {
}

Result<std::unique_ptr<FirmwarePackage>> RawFirmwareReader::read(const std::string& path) {
    auto data = readFile(path);
    if (data.empty()) {
        return static_cast<ErrorCode>(FirmwareError::InvalidFormat);
    }

    PackageMetadata meta;
    meta.vendor = "raw";
    meta.platform = "raw";
    std::string filename = fs::path(path).stem().string();
    
    auto pkg = std::make_unique<FirmwarePackage>(std::move(meta));
    
    FirmwareImageInfo info;
    info.name = filename;
    info.partitionName = m_partitionName;
    info.type = ImageType::Custom;
    info.format = ImageFormat::Raw;
    info.size = data.size();
    info.sourceFile = path;
    
    FirmwareImage image;
    image.info = info;
    image.data = std::move(data);
    image.dataLoaded = true;
    
    (void)pkg->addImage(std::move(image));
    
    PackageManifest manifest;
    manifest.images.push_back(info);
    pkg->setManifest(std::move(manifest));
    
    return pkg;
}

bool RawFirmwareReader::canRead(const std::string& path) const {
    fs::path p(path);
    if (fs::is_directory(p)) return false;
    auto ext = p.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    return ext == ".bin" || ext == ".img" || ext == ".elf" || ext == ".mbn" || ext.empty();
}

// ============================================================
// ZipFirmwareReader
// ============================================================

Result<std::unique_ptr<FirmwarePackage>> ZipFirmwareReader::read(const std::string& path) {
    (void)path;
    return static_cast<ErrorCode>(FirmwareError::InvalidFormat);
}

bool ZipFirmwareReader::canRead(const std::string& path) const {
    fs::path p(path);
    auto ext = p.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    return ext == ".zip";
}

} // namespace firmware
} // namespace mbootcore
