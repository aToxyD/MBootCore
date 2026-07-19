#include <mbootcore/dsp/DSPBuilder.hpp>
#include <mbootcore/dsp/DSPInspector.hpp>
#include <mbootcore/dsp/DSPValidator.hpp>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <algorithm>
#include <nlohmann/json.hpp>
#include <iomanip>
#include <ctime>
#include <zlib.h>

namespace fs = std::filesystem;

namespace mbootcore {
namespace dsp {

namespace {

std::string nowIso() {
    auto t = std::time(nullptr);
    auto tm = *std::gmtime(&t);
    std::ostringstream ss;
    ss << std::put_time(&tm, "%Y-%m-%dT%H:%M:%SZ");
    return ss.str();
}

uint32_t computeFileCrc32(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) return 0;
    uint32_t crc = 0xFFFFFFFF;
    char buf[4096];
    static const uint32_t table[256] = {
        0x00000000,0x77073096,0xEE0E612C,0x990951BA,0x076DC419,0x706AF48F,0xE963A535,0x9E6495A3,
        0x0EDB8832,0x79DCB8A4,0xE0D5E91E,0x97D2D988,0x09B64C2B,0x7EB17CBD,0xE7B82D07,0x90BF1D91,
        0x1DB71064,0x6AB020F2,0xF3B97148,0x84BE41DE,0x1ADAD47D,0x6DDDE4EB,0xF4D4B551,0x83D385C7,
        0x136C9856,0x646BA8C0,0xFD62F97A,0x8A65C9EC,0x14015C4F,0x63066CD9,0xFA0F3D63,0x8D080DF5,
        0x3B6E20C8,0x4C69105E,0xD56041E4,0xA2677172,0x3C03E4D1,0x4B04D447,0xD20D85FD,0xA50AB56B,
        0x35B5A8FA,0x42B2986C,0xDBBBC9D6,0xACBCF940,0x32D86CE3,0x45DF5C75,0xDCD60DCF,0xABD13D59,
        0x26D930AC,0x51DE003A,0xC8D75180,0xBFD06116,0x21B4F4B5,0x56B3C423,0xCFBA9599,0xB8BDA50F,
        0x2802B89E,0x5F058808,0xC60CD9B2,0xB10BE924,0x2F6F7C87,0x58684C11,0xC1611DAB,0xB6662D3D,
        0x76DC4190,0x01DB7106,0x98D220BC,0xEFD5102A,0x71B18589,0x06B6B51F,0x9FBFE4A5,0xE8B8D433,
        0x7807C9A2,0x0F00F934,0x9609A88E,0xE10E9818,0x7F6A0DBB,0x086D3D2D,0x91646C97,0xE6635C01,
        0x6B6B51F4,0x1C6C6162,0x856530D8,0xF262004E,0x6C0695ED,0x1B01A57B,0x8208F4C1,0xF50FC457,
        0x65B0D9C6,0x12B7E950,0x8BBEB8EA,0xFCB9887C,0x62DD1DDF,0x15DA2D49,0x8CD37CF3,0xFBD44C65,
        0x4DB26158,0x3AB551CE,0xA3BC0074,0xD4BB30E2,0x4ADFA541,0x3DD895D7,0xA4D1C46D,0xD3D6F4FB,
        0x4369E96A,0x346ED9FC,0xAD678846,0xDA60B8D0,0x44042D73,0x33031DE5,0xAA0A4C5F,0xDD0D7CC9,
        0x5005713C,0x270241AA,0xBE0B1010,0xC90C2086,0x5768B525,0x206F85B3,0xB966D409,0xCE61E49F,
        0x5EDEF90E,0x29D9C998,0xB0D09822,0xC7D7A8B4,0x59B33D17,0x2EB40D81,0xB7BD5C3B,0xC0BA6CAD,
        0xEDB88320,0x9ABFB3B6,0x03B6E20C,0x74B1D29A,0xEAD54739,0x9DD277AF,0x04DB2615,0x73DC1683,
        0xE3630B12,0x94643B84,0x0D6D6A3E,0x7A6A5AA8,0xE40ECF0B,0x9309FF9D,0x0A00AE27,0x7D079EB1,
        0xF00F9344,0x8708A3D2,0x1E01F268,0x6906C2FE,0xF762575D,0x806567CB,0x196C3671,0x6E6B06E7,
        0xFED41B76,0x89D32BE0,0x10DA7A5A,0x67DD4ACC,0xF9B9DF6F,0x8EBEEFF9,0x17B7BE43,0x60B08ED5,
        0xD6D6A3E8,0xA1D1937E,0x38D8C2C4,0x4FDFF252,0xD1BB67F1,0xA6BC5767,0x3FB506DD,0x48B2364B,
        0xD80D2BDA,0xAF0A1B4C,0x36034AF6,0x41047A60,0xDF60EFC3,0xA867DF55,0x316E8EEF,0x4669BE79,
        0xCB61B38C,0xBC66831A,0x256FD2A0,0x5268E236,0xCC0C7795,0xBB0B4703,0x220216B9,0x5505262F,
        0xC5BA3BBE,0xB2BD0B28,0x2BB45A92,0x5CB36A04,0xC2D7FFA7,0xB5D0CF31,0x2CD99E8B,0x5BDEAE1D,
        0x9B64C2B0,0xEC63F226,0x756AA39C,0x026D930A,0x9C0906A9,0xEB0E363F,0x72076785,0x05005713,
        0x95BF4A82,0xE2B87A14,0x7BB12BAE,0x0CB61B38,0x92D28E9B,0xE5D5BE0D,0x7CDCEFB7,0x0BDBDF21,
        0x86D3D2D4,0xF1D4E242,0x68DDB3F8,0x1FDA836E,0x81BE16CD,0xF6B9265B,0x6FB077E1,0x18B74777,
        0x88085AE6,0xFF0F6A70,0x66063BCA,0x11010B5C,0x8F659EFF,0xF862AE69,0x616BFFD3,0x166CCF45,
        0xA00AE278,0xD70DD2EE,0x4E048354,0x3903B3C2,0xA7672661,0xD06016F7,0x4969474D,0x3E6E77DB,
        0xAED16A4A,0xD9D65ADC,0x40DF0B66,0x37D83BF0,0xA9BCAE53,0xDEBB9EC5,0x47B2CF7F,0x30B5FFE9,
        0xBDBDF21C,0xCABAC28A,0x53B39330,0x24B4A3A6,0xBAD03605,0xCDD70693,0x54DE5729,0x23D967BF,
        0xB3667A2E,0xC4614AB8,0x5D681B02,0x2A6F2B94,0xB40BBE37,0xC30C8EA1,0x5A05DF1B,0x2D02EF8D
    };
    while (file.read(buf, sizeof(buf)) || file.gcount() > 0) {
        auto n = static_cast<size_t>(file.gcount());
        for (size_t i = 0; i < n; ++i) {
            crc = table[(crc ^ static_cast<uint8_t>(buf[i])) & 0xFF] ^ (crc >> 8);
        }
    }
    return ~crc;
}


std::string sha256String(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) return {};
    uint32_t h[8] = {
        0x6A09E667, 0xBB67AE85, 0x3C6EF372, 0xA54FF53A,
        0x510E527F, 0x9B05688C, 0x1F83D9AB, 0x5BE0CD19
    };
    uint32_t k[64] = {
        0x428A2F98,0x71374491,0xB5C0FBCF,0xE9B5DBA5,0x3956C25B,0x59F111F1,0x923F82A4,0xAB1C5ED5,
        0xD807AA98,0x12835B01,0x243185BE,0x550C7DC3,0x72BE5D74,0x80DEB1FE,0x9BDC06A7,0xC19BF174,
        0xE49B69C1,0xEFBE4786,0x0FC19DC6,0x240CA1CC,0x2DE92C6F,0x4A7484AA,0x5CB0A9DC,0x76F988DA,
        0x983E5152,0xA831C66D,0xB00327C8,0xBF597FC7,0xC6E00BF3,0xD5A79147,0x06CA6351,0x14292967,
        0x27B70A85,0x2E1B2138,0x4D2C6DFC,0x53380D13,0x650A7354,0x766A0ABB,0x81C2C92E,0x92722C85,
        0xA2BFE8A1,0xA81A664B,0xC24B8B70,0xC76C51A3,0xD192E819,0xD6990624,0xF40E3585,0x106AA070,
        0x19A4C116,0x1E376C08,0x2748774C,0x34B0BCB5,0x391C0CB3,0x4ED8AA4A,0x5B9CCA4F,0x682E6FF3,
        0x748F82EE,0x78A5636F,0x84C87814,0x8CC70208,0x90BEFFFA,0xA4506CEB,0xBEF9A3F7,0xC67178F2
    };
    auto rr = [](uint32_t x, uint32_t n) { return (x >> n) | (x << (32 - n)); };
    char buf[64];
    uint64_t bitLen = 0;
    int bufLen = 0;
    auto process = [&](const uint8_t* data, size_t len) {
        for (size_t i = 0; i < len; ++i) {
            buf[bufLen++] = static_cast<char>(data[i]);
            if (bufLen == 64) {
                uint32_t w[64];
                for (int t = 0; t < 16; ++t) {
                    w[t] = (static_cast<uint8_t>(buf[t*4]) << 24) |
                           (static_cast<uint8_t>(buf[t*4+1]) << 16) |
                           (static_cast<uint8_t>(buf[t*4+2]) << 8) |
                           static_cast<uint8_t>(buf[t*4+3]);
                }
                for (int t = 16; t < 64; ++t) {
                    uint32_t s0 = rr(w[t-15], 7) ^ rr(w[t-15], 18) ^ (w[t-15] >> 3);
                    uint32_t s1 = rr(w[t-2], 17) ^ rr(w[t-2], 19) ^ (w[t-2] >> 10);
                    w[t] = w[t-16] + s0 + w[t-7] + s1;
                }
                uint32_t a = h[0], b = h[1], c = h[2], d = h[3];
                uint32_t e = h[4], f = h[5], g = h[6], hh = h[7];
                for (int t = 0; t < 64; ++t) {
                    uint32_t S1 = rr(e, 6) ^ rr(e, 11) ^ rr(e, 25);
                    uint32_t ch = (e & f) ^ ((~e) & g);
                    uint32_t temp1 = hh + S1 + ch + k[t] + w[t];
                    uint32_t S0 = rr(a, 2) ^ rr(a, 13) ^ rr(a, 22);
                    uint32_t maj = (a & b) ^ (a & c) ^ (b & c);
                    uint32_t temp2 = S0 + maj;
                    hh = g; g = f; f = e; e = d + temp1;
                    d = c; c = b; b = a; a = temp1 + temp2;
                }
                h[0] += a; h[1] += b; h[2] += c; h[3] += d;
                h[4] += e; h[5] += f; h[6] += g; h[7] += hh;
                bufLen = 0;
            }
            bitLen += 8;
        }
    };
    char fileBuf[4096];
    while (file.read(fileBuf, sizeof(fileBuf)) || file.gcount() > 0) {
        process(reinterpret_cast<const uint8_t*>(fileBuf), static_cast<size_t>(file.gcount()));
    }
    process(reinterpret_cast<const uint8_t*>("\x80"), 1);
    while (bufLen != 56) {
        uint8_t z = 0;
        process(&z, 1);
    }
    for (int i = 7; i >= 0; --i) {
        uint8_t b = static_cast<uint8_t>((bitLen >> (i * 8)) & 0xFF);
        process(&b, 1);
    }
    std::ostringstream hex;
    for (int i = 0; i < 8; ++i) {
        hex << std::hex << std::setw(8) << std::setfill('0') << h[i];
    }
    return hex.str();
}

bool copyDirectory(const fs::path& src, const fs::path& dst) {
    try {
        fs::create_directories(dst);
        for (auto& entry : fs::recursive_directory_iterator(src)) {
            auto rel = fs::relative(entry.path(), src);
            auto target = dst / rel;
            if (entry.is_directory()) {
                fs::create_directories(target);
            } else if (entry.is_regular_file()) {
                fs::copy_file(entry.path(), target, fs::copy_options::overwrite_existing);
            }
        }
        return true;
    } catch (...) {
        return false;
    }
}

} // anonymous namespace

struct DSPBuilder::Impl {
    BuildConfig m_config;
    std::vector<std::string> m_generatedFiles;
    std::vector<std::string> m_warnings;
    DSPValidator m_validator;

    std::string packageDir() const {
        return (fs::path(m_config.outputDir) / m_config.packageId).string();
    }

    Result<void> doGenerateManifest(const BuildConfig& cfg, const std::string& outputPath);
    Result<void> doGenerateChecksums(const std::string& pkgPath);
    Result<void> doGenerateDependencies(const std::string& pkgPath);
    Result<void> doCompressLoaders(const std::string& pkgPath);
    Result<void> doGenerateChecksumManifest(const std::string& pkgPath);
    Result<void> writeJsonFile(const std::string& path, const std::string& content);
    bool isFileExcluded(const std::string& relPath) const;
};

bool DSPBuilder::Impl::isFileExcluded(const std::string& relPath) const {
    for (const auto& pat : m_config.excludePatterns) {
        if (relPath.find(pat) != std::string::npos) return true;
    }
    if (!m_config.includePatterns.empty()) {
        for (const auto& pat : m_config.includePatterns) {
            if (relPath.find(pat) != std::string::npos) return true;
        }
        return false;
    }
    return false;
}

Result<void> DSPBuilder::Impl::writeJsonFile(const std::string& path, const std::string& content) {
    fs::create_directories(fs::path(path).parent_path());
    std::ofstream out(path);
    if (!out) {
        return ErrorCode::InvalidArgument;
    }
    out << content;
    out.close();
    m_generatedFiles.push_back(path);
    return {};
}

Result<void> DSPBuilder::Impl::doGenerateManifest(const BuildConfig& cfg, const std::string& outputPath) {
    try {
        nlohmann::json j;
        j["formatVersion"] = "1.0";
        j["packageId"] = cfg.packageId;
        j["name"] = cfg.name;
        j["vendor"] = cfg.vendor;
        j["description"] = cfg.description;
        j["version"] = cfg.version;
        j["authors"] = cfg.authors;
        j["license"] = cfg.license;
        j["buildDate"] = nowIso();
        j["compressLoaders"] = cfg.compressLoaders;
        j["checksumAlgorithm"] = "SHA256";
        return writeJsonFile(outputPath, j.dump(2));
    } catch (...) {
        return ErrorCode::Unknown;
    }
}

Result<void> DSPBuilder::Impl::doGenerateChecksums(const std::string& pkgPath) {
    auto checksumPath = fs::path(pkgPath) / "checksum.sha256";
    std::ofstream out(checksumPath);
    if (!out) {
        return ErrorCode::InvalidArgument;
    }

    auto iter = fs::recursive_directory_iterator(pkgPath);
    for (auto& entry : iter) {
        if (!entry.is_regular_file()) continue;
        auto rel = fs::relative(entry.path(), pkgPath).string();
        if (rel == "checksum.sha256" || rel == "signature.sig") continue;
        auto hash = sha256String(entry.path().string());
        if (hash.empty()) continue;
        out << hash << "  " << rel << "\n";
    }
    out.close();
    m_generatedFiles.push_back(checksumPath.string());
    return {};
}

Result<void> DSPBuilder::Impl::doGenerateDependencies(const std::string& pkgPath) {
    auto depsPath = fs::path(pkgPath) / "dependencies.json";
    try {
        nlohmann::json j;
        j["dependencies"] = nlohmann::json::array();
        // Read dependencies from manifest
        auto manifestPath = fs::path(pkgPath) / "manifest.json";
        if (fs::exists(manifestPath)) {
            std::ifstream mf(manifestPath);
            if (mf) {
                try {
                    auto manifest = nlohmann::json::parse(mf);
                    auto deps = manifest.value("dependencies", nlohmann::json::array());
                    if (deps.is_array()) {
                        j["dependencies"] = deps;
                    }
                } catch (...) {}
            }
        }
        j["resolved"] = true;
        return writeJsonFile(depsPath.string(), j.dump(2));
    } catch (...) {
        return ErrorCode::Unknown;
    }
}

Result<void> DSPBuilder::Impl::doCompressLoaders(const std::string& pkgPath) {
    auto loadersDir = fs::path(pkgPath) / "loaders";
    if (!fs::exists(loadersDir)) {
        return {};
    }
    for (auto& entry : fs::directory_iterator(loadersDir)) {
        if (!entry.is_regular_file()) continue;
        auto ext = entry.path().extension().string();
        if (ext == ".gz") continue;
        auto inPath = entry.path().string();
        auto outPath = inPath + ".gz";
        std::ifstream inFile(inPath, std::ios::binary);
        if (!inFile) {
            m_warnings.push_back("Cannot open loader: " + inPath);
            continue;
        }
        std::vector<char> inData((std::istreambuf_iterator<char>(inFile)),
                                  std::istreambuf_iterator<char>());
        inFile.close();
        if (inData.empty()) continue;
        std::vector<char> outData;
        outData.reserve(inData.size() + 64);
        outData.push_back(static_cast<char>(0x1F));
        outData.push_back(static_cast<char>(0x8B));
        outData.push_back(static_cast<char>(0x08));
        outData.push_back(static_cast<char>(0x00));
        outData.push_back(static_cast<char>(0x00));
        outData.push_back(static_cast<char>(0x00));
        outData.push_back(0x00);
        outData.push_back(0x00);
        outData.push_back(0x00);
        outData.push_back(0x03);
        auto ptr = reinterpret_cast<const uint8_t*>(inData.data());
        auto len = inData.size();
        auto deflateBound = len + (len / 999) + 12;
        std::vector<uint8_t> deflated(deflateBound);
        uLongf destLen = static_cast<uLongf>(deflated.size());
        auto crc = computeFileCrc32(entry.path().string());
        if (compress2(deflated.data(), &destLen, ptr, static_cast<uLong>(len), Z_BEST_COMPRESSION) == Z_OK) {
            for (size_t i = 0; i < destLen; ++i) {
                outData.push_back(static_cast<char>(deflated[i]));
            }
            outData.push_back(static_cast<char>((crc >> 0) & 0xFF));
            outData.push_back(static_cast<char>((crc >> 8) & 0xFF));
            outData.push_back(static_cast<char>((crc >> 16) & 0xFF));
            outData.push_back(static_cast<char>((crc >> 24) & 0xFF));
            outData.push_back(static_cast<char>((len >> 0) & 0xFF));
            outData.push_back(static_cast<char>((len >> 8) & 0xFF));
            outData.push_back(static_cast<char>((len >> 16) & 0xFF));
            outData.push_back(static_cast<char>((len >> 24) & 0xFF));
            std::ofstream outFile(outPath, std::ios::binary);
            if (outFile) {
                outFile.write(outData.data(), static_cast<std::streamsize>(outData.size()));
                m_generatedFiles.push_back(outPath);
            }
        }
    }
    return {};
}

Result<void> DSPBuilder::Impl::doGenerateChecksumManifest(const std::string& pkgPath) {
    auto sigPath = fs::path(pkgPath) / "signature.sig";
    std::ofstream out(sigPath);
    if (!out) {
        return ErrorCode::InvalidArgument;
    }

    out << "-------------------------------------------------\n";
    out << "\n";
    out << "MBoot DSP Package\n";
    out << "\n";
    out << "Checksum Manifest\n";
    out << "\n";
    out << "Algorithm:\n";
    out << "SHA-256\n";
    out << "\n";
    out << "Purpose:\n";
    out << "Integrity verification only.\n";
    out << "This file is NOT a cryptographic signature.\n";
    out << "\n";
    out << "-------------------------------------------------\n";
    out << "\n";
    out << "Package: " << fs::path(pkgPath).filename().string() << "\n";
    out << "Generated: " << nowIso() << "\n";
    out << "\n";

    auto checksumPath = fs::path(pkgPath) / "checksum.sha256";
    if (fs::exists(checksumPath)) {
        std::ifstream cs(checksumPath);
        std::stringstream ss;
        ss << cs.rdbuf();
        auto content = ss.str();
        out << "SHA256 digest of checksum.sha256:\n";
        auto digest = sha256String(checksumPath.string());
        out << digest << "\n";
    } else {
        auto digest = sha256String(fs::path(pkgPath).string());
        out << "SHA256 digest of package directory:\n";
        out << digest << "\n";
    }
    out.close();
    m_generatedFiles.push_back(sigPath.string());
    return {};
}

DSPBuilder::DSPBuilder()
    : m_impl(std::make_unique<Impl>()) {
}

DSPBuilder::~DSPBuilder() = default;

Result<void> DSPBuilder::build(const BuildConfig& config) {
    m_impl->m_config = config;
    m_impl->m_generatedFiles.clear();
    m_impl->m_warnings.clear();

    auto srcDir = fs::path(config.sourceDir);
    auto outDir = fs::path(config.outputDir);

    if (!fs::exists(srcDir)) {
        return ErrorCode::InvalidArgument;
    }
    fs::create_directories(outDir);

    auto pkgPath = m_impl->packageDir();
    if (!copyDirectory(srcDir, pkgPath)) {
        return ErrorCode::Unknown;
    }

    if (config.generateManifest) {
        auto manifestPath = fs::path(pkgPath) / "manifest.json";
        auto result = m_impl->doGenerateManifest(config, manifestPath.string());
        if (result.isError()) return result;
    }

    auto loadersSrc = srcDir / "loaders";
    auto loadersDst = fs::path(pkgPath) / "loaders";
    if (fs::exists(loadersSrc)) {
        fs::create_directories(loadersDst);
        for (auto& entry : fs::directory_iterator(loadersSrc)) {
            if (entry.is_regular_file()) {
                auto dest = loadersDst / entry.path().filename();
                fs::copy_file(entry.path(), dest, fs::copy_options::overwrite_existing);
            }
        }
    }

    auto chipsetPath = fs::path(pkgPath) / "chipsets.json";
    {
        nlohmann::json j;
        j["chipsets"] = nlohmann::json::array();
        auto result = m_impl->writeJsonFile(chipsetPath.string(), j.dump(2));
        if (result.isError()) return result;
    }

    auto vendorPath = fs::path(pkgPath) / "vendor.json";
    {
        nlohmann::json j;
        j["vendor"] = config.vendor;
        j["name"] = config.name;
        auto result = m_impl->writeJsonFile(vendorPath.string(), j.dump(2));
        if (result.isError()) return result;
    }

    auto profilesSrc = srcDir / "profiles";
    auto profilesDst = fs::path(pkgPath) / "profiles";
    if (fs::exists(profilesSrc)) {
        fs::create_directories(profilesDst);
        for (auto& entry : fs::recursive_directory_iterator(profilesSrc)) {
            auto rel = fs::relative(entry.path(), profilesSrc);
            auto target = profilesDst / rel;
            if (entry.is_directory()) {
                fs::create_directories(target);
            } else if (entry.is_regular_file()) {
                fs::copy_file(entry.path(), target, fs::copy_options::overwrite_existing);
            }
        }
    }

    auto quirksSrc = srcDir / "quirks";
    auto quirksDst = fs::path(pkgPath) / "quirks";
    if (fs::exists(quirksSrc)) {
        fs::create_directories(quirksDst);
        for (auto& entry : fs::recursive_directory_iterator(quirksSrc)) {
            auto rel = fs::relative(entry.path(), quirksSrc);
            auto target = quirksDst / rel;
            if (entry.is_directory()) {
                fs::create_directories(target);
            } else if (entry.is_regular_file()) {
                fs::copy_file(entry.path(), target, fs::copy_options::overwrite_existing);
            }
        }
    }

    if (config.generateChecksum) {
        auto result = m_impl->doGenerateChecksums(pkgPath);
        if (result.isError()) return result;
    }

    if (config.compressLoaders) {
        auto result = m_impl->doCompressLoaders(pkgPath);
        if (result.isError()) return result;
    }

    if (config.generateChecksumManifest) {
        auto result = m_impl->doGenerateChecksumManifest(pkgPath);
        if (result.isError()) return result;
    }

    return {};
}

Result<void> DSPBuilder::buildFromManifest(const std::string& manifestPath, const std::string& outputDir) {
    fs::path mfPath(manifestPath);
    if (!fs::exists(mfPath)) {
        return ErrorCode::Unknown;
    }
    std::ifstream mf(manifestPath);
    if (!mf) {
        return ErrorCode::Unknown;
    }
    try {
        auto manifest = nlohmann::json::parse(mf);
        BuildConfig cfg;
        cfg.outputDir = outputDir;
        cfg.packageId = manifest.value("packageId", std::string{});
        cfg.name = manifest.value("name", std::string{});
        cfg.vendor = manifest.value("vendor", std::string{});
        cfg.description = manifest.value("description", std::string{});
        cfg.version = manifest.value("version", std::string{});
        cfg.license = manifest.value("license", std::string{});
        auto srcDir = fs::path(manifestPath).parent_path().string();
        cfg.sourceDir = srcDir;
        return build(cfg);
    } catch (...) {
        return ErrorCode::Unknown;
    }
}

Result<void> DSPBuilder::generateManifest(const BuildConfig& config, const std::string& outputPath) {
    return m_impl->doGenerateManifest(config, outputPath);
}

Result<void> DSPBuilder::generateChecksums(const std::string& packagePath) {
    return m_impl->doGenerateChecksums(packagePath);
}

Result<void> DSPBuilder::generateDependencies(const std::string& packagePath) {
    return m_impl->doGenerateDependencies(packagePath);
}

Result<void> DSPBuilder::compressLoaders(const std::string& packagePath) {
    return m_impl->doCompressLoaders(packagePath);
}

Result<void> DSPBuilder::generateChecksumManifest(const std::string& packagePath) {
    return m_impl->doGenerateChecksumManifest(packagePath);
}

Result<void> DSPBuilder::validateBuiltPackage(const std::string& packagePath) {
    auto report = m_impl->m_validator.validate(packagePath, DSPValidationLevel::Full);
    if (!report.valid) {
        return ErrorCode::Unknown;
    }
    return {};
}

const BuildConfig& DSPBuilder::config() const noexcept {
    return m_impl->m_config;
}

std::vector<std::string> DSPBuilder::generatedFiles() const noexcept {
    return m_impl->m_generatedFiles;
}

std::vector<std::string> DSPBuilder::warnings() const noexcept {
    return m_impl->m_warnings;
}

} // namespace dsp
} // namespace mbootcore
