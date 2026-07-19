#include "mbootcore/dsp/DSPCache.hpp"
#include "mbootcore/domain/Error.hpp"

#include <algorithm>
#include <chrono>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <unordered_map>

namespace mbootcore {
namespace dsp {

namespace {

namespace fs = std::filesystem;
using namespace std::chrono;

constexpr const char* kMetaExt = ".meta";
constexpr const char* kLoaderExt = ".ldr";
constexpr const char* kLookupExt = ".lup";
constexpr const char* kDepExt = ".dep";

std::string cacheFilePathFor(const std::string& cachePath, const std::string& id,
                              const char* ext) {
    return (fs::path(cachePath) / (id + ext)).string();
}

std::string sanitizeId(const std::string& id) {
    std::string result;
    result.reserve(id.size());
    for (char c : id) {
        if (std::isalnum(static_cast<unsigned char>(c)) || c == '_' || c == '-' || c == '.') {
            result.push_back(c);
        } else {
            result.push_back('_');
        }
    }
    return result;
}

bool writeBinaryFile(const std::string& path, const uint8_t* data, size_t size) {
    std::ofstream ofs(path, std::ios::binary);
    if (!ofs) return false;
    ofs.write(reinterpret_cast<const char*>(data), static_cast<std::streamsize>(size));
    return ofs.good();
}

ByteBuffer readBinaryFile(const std::string& path) {
    std::ifstream ifs(path, std::ios::binary | std::ios::ate);
    if (!ifs) return {};
    std::streamsize size = ifs.tellg();
    if (size <= 0) return {};
    ifs.seekg(0, std::ios::beg);
    ByteBuffer buf(static_cast<size_t>(size));
    if (!ifs.read(reinterpret_cast<char*>(buf.data()), size)) return {};
    return buf;
}

// Binary serialization helpers for DSPPackageMetadata
// Format: [field_count:u32] [key_len:u32][key_data][val_len:u32][val_data] ...


void serializeString(const std::string& s, ByteBuffer& out) {
    uint32_t len = static_cast<uint32_t>(s.size());
    auto pos = out.size();
    out.resize(out.size() + sizeof(uint32_t) + len);
    std::memcpy(out.data() + pos, &len, sizeof(uint32_t));
    if (len > 0) {
        std::memcpy(out.data() + pos + sizeof(uint32_t), s.data(), len);
    }
}

void serializeVersion(const DSPVersion& v, ByteBuffer& out) {
    auto pos = out.size();
    out.resize(out.size() + sizeof(uint32_t) * 3);
    std::memcpy(out.data() + pos, &v.major, sizeof(uint32_t));
    std::memcpy(out.data() + pos + sizeof(uint32_t), &v.minor, sizeof(uint32_t));
    std::memcpy(out.data() + pos + sizeof(uint32_t) * 2, &v.patch, sizeof(uint32_t));
}



bool deserializeString(const uint8_t* data, size_t& pos, size_t size, std::string& out) {
    if (pos + sizeof(uint32_t) > size) return false;
    uint32_t len;
    std::memcpy(&len, data + pos, sizeof(uint32_t));
    pos += sizeof(uint32_t);
    if (pos + len > size) return false;
    out.assign(reinterpret_cast<const char*>(data + pos), len);
    pos += len;
    return true;
}

bool deserializeVersion(const uint8_t* data, size_t& pos, size_t size, DSPVersion& v) {
    if (pos + sizeof(uint32_t) * 3 > size) return false;
    std::memcpy(&v.major, data + pos, sizeof(uint32_t));
    std::memcpy(&v.minor, data + pos + sizeof(uint32_t), sizeof(uint32_t));
    std::memcpy(&v.patch, data + pos + sizeof(uint32_t) * 2, sizeof(uint32_t));
    pos += sizeof(uint32_t) * 3;
    return true;
}

DSPVersion cachedVersion{1, 0, 0};
constexpr uint32_t kCacheFormatMagic = 0x4443504D; // "DSPM"

} // anonymous namespace

struct DSPCache::Impl {
    std::string m_cachePath;
    mutable uint64_t m_sizeBytes{0};
    mutable size_t m_entryCount{0};
    mutable bool m_statsStale{true};

    explicit Impl(const std::string& path)
        : m_cachePath(path) {}

    void ensureDir() const {
        if (!m_cachePath.empty()) {
            fs::create_directories(fs::path(m_cachePath));
        }
    }

    void refreshStats() const {
        m_sizeBytes = 0;
        m_entryCount = 0;
        if (!fs::exists(fs::path(m_cachePath))) {
            m_statsStale = false;
            return;
        }
        for (const auto& entry : fs::directory_iterator(fs::path(m_cachePath))) {
            if (entry.is_regular_file()) {
                m_sizeBytes += static_cast<uint64_t>(entry.file_size());
                m_entryCount++;
            }
        }
        m_statsStale = false;
    }
};

DSPCache::DSPCache(const std::string& cachePath)
    : m_impl(std::make_unique<Impl>(cachePath)) {}

DSPCache::~DSPCache() = default;

// --- Metadata cache ---

Result<void> DSPCache::cacheMetadata(const std::string& packageId, const DSPPackageMetadata& metadata) {
    m_impl->ensureDir();
    std::string safeId = sanitizeId(packageId);
    std::string path = cacheFilePathFor(m_impl->m_cachePath, safeId, kMetaExt);

    ByteBuffer buf;

    // Format magic + version
    auto pos = buf.size();
    buf.resize(buf.size() + sizeof(uint32_t) + sizeof(DSPVersion));
    std::memcpy(buf.data() + pos, &kCacheFormatMagic, sizeof(uint32_t));
    std::memcpy(buf.data() + pos + sizeof(uint32_t), &cachedVersion, sizeof(DSPVersion));

    // Manifest fields
    serializeString(metadata.manifest.packageId, buf);
    serializeString(metadata.manifest.name, buf);
    serializeString(metadata.manifest.vendor, buf);
    serializeString(metadata.manifest.description, buf);
    serializeVersion(metadata.manifest.version, buf);
    serializeString(metadata.manifest.minCoreVersion, buf);
    serializeString(metadata.manifest.maxCoreVersion, buf);

    // State + Origin
    uint32_t state = static_cast<uint32_t>(metadata.state);
    uint32_t origin = static_cast<uint32_t>(metadata.origin);
    pos = buf.size();
    buf.resize(buf.size() + sizeof(uint32_t) * 2);
    std::memcpy(buf.data() + pos, &state, sizeof(uint32_t));
    std::memcpy(buf.data() + pos + sizeof(uint32_t), &origin, sizeof(uint32_t));

    serializeString(metadata.installPath, buf);
    serializeString(metadata.packageFileName, buf);
    serializeString(metadata.repository, buf);

    // Chipsets
    uint32_t chipsetCount = static_cast<uint32_t>(metadata.chipsets.size());
    pos = buf.size();
    buf.resize(buf.size() + sizeof(uint32_t));
    std::memcpy(buf.data() + pos, &chipsetCount, sizeof(uint32_t));
    for (const auto& cs : metadata.chipsets) {
        serializeString(cs.id.vendor, buf);
        serializeString(cs.id.family, buf);
        serializeString(cs.id.variant, buf);
        serializeString(cs.displayName, buf);
    }

    if (!writeBinaryFile(path, buf.data(), buf.size())) {
        return ErrorCode::Unknown;
    }

    m_impl->m_statsStale = true;
    return {};
}

Result<DSPPackageMetadata> DSPCache::loadMetadata(const std::string& packageId) const {
    std::string safeId = sanitizeId(packageId);
    std::string path = cacheFilePathFor(m_impl->m_cachePath, safeId, kMetaExt);

    ByteBuffer buf = readBinaryFile(path);
    if (buf.empty()) {
        return ErrorCode::CacheMiss;
    }

    size_t pos = 0;

    uint32_t magic;
    if (pos + sizeof(uint32_t) > buf.size()) {
        return ErrorCode::Unknown;
    }
    std::memcpy(&magic, buf.data() + pos, sizeof(uint32_t));
    pos += sizeof(uint32_t);
    if (magic != kCacheFormatMagic) {
        return ErrorCode::Unknown;
    }

    DSPVersion fmtVer;
    if (!deserializeVersion(buf.data(), pos, buf.size(), fmtVer)) {
        return ErrorCode::Unknown;
    }

    DSPPackageMetadata meta;

    if (!deserializeString(buf.data(), pos, buf.size(), meta.manifest.packageId)) {
        return ErrorCode::Unknown;
    }
    if (!deserializeString(buf.data(), pos, buf.size(), meta.manifest.name)) {
        return ErrorCode::Unknown;
    }
    if (!deserializeString(buf.data(), pos, buf.size(), meta.manifest.vendor)) {
        return ErrorCode::Unknown;
    }
    if (!deserializeString(buf.data(), pos, buf.size(), meta.manifest.description)) {
        return ErrorCode::Unknown;
    }
    if (!deserializeVersion(buf.data(), pos, buf.size(), meta.manifest.version)) {
        return ErrorCode::Unknown;
    }
    if (!deserializeString(buf.data(), pos, buf.size(), meta.manifest.minCoreVersion)) {
        return ErrorCode::Unknown;
    }
    if (!deserializeString(buf.data(), pos, buf.size(), meta.manifest.maxCoreVersion)) {
        return ErrorCode::Unknown;
    }

    if (pos + sizeof(uint32_t) * 2 > buf.size()) {
        return ErrorCode::Unknown;
    }
    uint32_t state, origin;
    std::memcpy(&state, buf.data() + pos, sizeof(uint32_t));
    pos += sizeof(uint32_t);
    std::memcpy(&origin, buf.data() + pos, sizeof(uint32_t));
    pos += sizeof(uint32_t);
    meta.state = static_cast<DSPState>(state);
    meta.origin = static_cast<DSPOrigin>(origin);

    if (!deserializeString(buf.data(), pos, buf.size(), meta.installPath)) {
        return ErrorCode::Unknown;
    }
    if (!deserializeString(buf.data(), pos, buf.size(), meta.packageFileName)) {
        return ErrorCode::Unknown;
    }
    if (!deserializeString(buf.data(), pos, buf.size(), meta.repository)) {
        return ErrorCode::Unknown;
    }

    // Chipsets
    if (pos + sizeof(uint32_t) > buf.size()) {
        return ErrorCode::Unknown;
    }
    uint32_t chipsetCount;
    std::memcpy(&chipsetCount, buf.data() + pos, sizeof(uint32_t));
    pos += sizeof(uint32_t);
    for (uint32_t i = 0; i < chipsetCount; ++i) {
        DSPChipsetMetadata cs;
        if (!deserializeString(buf.data(), pos, buf.size(), cs.id.vendor)) break;
        if (!deserializeString(buf.data(), pos, buf.size(), cs.id.family)) break;
        if (!deserializeString(buf.data(), pos, buf.size(), cs.id.variant)) break;
        if (!deserializeString(buf.data(), pos, buf.size(), cs.displayName)) break;
        meta.chipsets.push_back(std::move(cs));
    }

    return meta;
}

bool DSPCache::hasMetadata(const std::string& packageId) const noexcept {
    std::string safeId = sanitizeId(packageId);
    std::string path = cacheFilePathFor(m_impl->m_cachePath, safeId, kMetaExt);
    return fs::exists(fs::path(path));
}

Result<void> DSPCache::invalidateMetadata(const std::string& packageId) {
    std::string safeId = sanitizeId(packageId);
    std::string path = cacheFilePathFor(m_impl->m_cachePath, safeId, kMetaExt);
    std::error_code ec;
    if (fs::remove(fs::path(path), ec)) {
        m_impl->m_statsStale = true;
        return {};
    }
    if (ec) {
        return ErrorCode::Unknown;
    }
    return {};
}

// --- Loader cache ---

Result<void> DSPCache::cacheLoader(const std::string& loaderId, const ByteBuffer& data) {
    m_impl->ensureDir();
    std::string safeId = sanitizeId(loaderId);
    std::string path = cacheFilePathFor(m_impl->m_cachePath, safeId, kLoaderExt);

    if (!writeBinaryFile(path, data.data(), data.size())) {
        return ErrorCode::Unknown;
    }

    m_impl->m_statsStale = true;
    return {};
}

Result<ByteBuffer> DSPCache::loadLoader(const std::string& loaderId) const {
    std::string safeId = sanitizeId(loaderId);
    std::string path = cacheFilePathFor(m_impl->m_cachePath, safeId, kLoaderExt);

    ByteBuffer data = readBinaryFile(path);
    if (data.empty() && !fs::exists(fs::path(path))) {
        return ErrorCode::CacheMiss;
    }
    return data;
}

bool DSPCache::hasLoader(const std::string& loaderId) const noexcept {
    std::string safeId = sanitizeId(loaderId);
    std::string path = cacheFilePathFor(m_impl->m_cachePath, safeId, kLoaderExt);
    return fs::exists(fs::path(path));
}

Result<void> DSPCache::invalidateLoader(const std::string& loaderId) {
    std::string safeId = sanitizeId(loaderId);
    std::string path = cacheFilePathFor(m_impl->m_cachePath, safeId, kLoaderExt);
    std::error_code ec;
    if (fs::remove(fs::path(path), ec)) {
        m_impl->m_statsStale = true;
        return {};
    }
    return {};
}

// --- Lookup cache ---

Result<void> DSPCache::cacheLookup(const std::string& key, const std::string& value) {
    m_impl->ensureDir();
    std::string safeKey = sanitizeId(key);
    std::string path = cacheFilePathFor(m_impl->m_cachePath, safeKey, kLookupExt);

    ByteBuffer buf;
    serializeString(value, buf);

    if (!writeBinaryFile(path, buf.data(), buf.size())) {
        return ErrorCode::Unknown;
    }

    m_impl->m_statsStale = true;
    return {};
}

Result<std::string> DSPCache::lookupCached(const std::string& key) const {
    std::string safeKey = sanitizeId(key);
    std::string path = cacheFilePathFor(m_impl->m_cachePath, safeKey, kLookupExt);

    ByteBuffer buf = readBinaryFile(path);
    if (buf.empty()) {
        return ErrorCode::CacheMiss;
    }

    size_t pos = 0;
    std::string value;
    if (!deserializeString(buf.data(), pos, buf.size(), value)) {
        return ErrorCode::Unknown;
    }
    return value;
}

bool DSPCache::hasLookup(const std::string& key) const noexcept {
    std::string safeKey = sanitizeId(key);
    std::string path = cacheFilePathFor(m_impl->m_cachePath, safeKey, kLookupExt);
    return fs::exists(fs::path(path));
}

// --- Dependency resolution cache ---

Result<void> DSPCache::cacheDependencyResolution(const std::string& packageId,
    const std::vector<std::string>& order) {
    m_impl->ensureDir();
    std::string safeId = sanitizeId(packageId);
    std::string path = cacheFilePathFor(m_impl->m_cachePath, safeId, kDepExt);

    ByteBuffer buf;
    uint32_t count = static_cast<uint32_t>(order.size());
    auto pos = buf.size();
    buf.resize(buf.size() + sizeof(uint32_t));
    std::memcpy(buf.data() + pos, &count, sizeof(uint32_t));

    for (const auto& id : order) {
        serializeString(id, buf);
    }

    if (!writeBinaryFile(path, buf.data(), buf.size())) {
        return ErrorCode::Unknown;
    }

    m_impl->m_statsStale = true;
    return {};
}

Result<std::vector<std::string>> DSPCache::loadDependencyResolution(const std::string& packageId) const {
    std::string safeId = sanitizeId(packageId);
    std::string path = cacheFilePathFor(m_impl->m_cachePath, safeId, kDepExt);

    ByteBuffer buf = readBinaryFile(path);
    if (buf.empty()) {
        return ErrorCode::CacheMiss;
    }

    size_t pos = 0;
    if (pos + sizeof(uint32_t) > buf.size()) {
        return ErrorCode::Unknown;
    }
    uint32_t count;
    std::memcpy(&count, buf.data() + pos, sizeof(uint32_t));
    pos += sizeof(uint32_t);

    std::vector<std::string> order;
    order.reserve(count);
    for (uint32_t i = 0; i < count; ++i) {
        std::string id;
        if (!deserializeString(buf.data(), pos, buf.size(), id)) {
            return ErrorCode::Unknown;
        }
        order.push_back(std::move(id));
    }

    return order;
}

// --- Cache management ---

Result<void> DSPCache::clear() {
    if (!fs::exists(fs::path(m_impl->m_cachePath))) {
        return {};
    }

    std::error_code ec;
    for (const auto& entry : fs::directory_iterator(fs::path(m_impl->m_cachePath), ec)) {
        if (entry.is_regular_file()) {
            fs::remove(entry.path(), ec);
        }
    }

    m_impl->m_sizeBytes = 0;
    m_impl->m_entryCount = 0;
    m_impl->m_statsStale = false;
    return {};
}

Result<void> DSPCache::clearAll() {
    return clear();
}

Result<void> DSPCache::prune(const std::chrono::hours& maxAge) {
    if (!fs::exists(fs::path(m_impl->m_cachePath))) {
        return {};
    }

    auto now = std::chrono::system_clock::now();
    std::error_code ec;

    for (const auto& entry : fs::directory_iterator(fs::path(m_impl->m_cachePath), ec)) {
        if (!entry.is_regular_file()) continue;

        auto lastWrite = entry.last_write_time(ec);
        if (ec) continue;

        auto tp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
            lastWrite - fs::file_time_type::clock::now() + std::chrono::system_clock::now());
        auto age = std::chrono::duration_cast<std::chrono::hours>(now - tp);

        if (age > maxAge) {
            fs::remove(entry.path(), ec);
        }
    }

    m_impl->m_statsStale = true;
    return {};
}

Result<void> DSPCache::rebuild() {
    m_impl->ensureDir();
    m_impl->m_statsStale = true;
    m_impl->refreshStats();
    return {};
}

uint64_t DSPCache::cacheSizeBytes() const noexcept {
    if (m_impl->m_statsStale) {
        m_impl->refreshStats();
    }
    return m_impl->m_sizeBytes;
}

size_t DSPCache::cacheEntryCount() const noexcept {
    if (m_impl->m_statsStale) {
        m_impl->refreshStats();
    }
    return m_impl->m_entryCount;
}

bool DSPCache::isWarm() const noexcept {
    if (m_impl->m_statsStale) {
        m_impl->refreshStats();
    }
    return m_impl->m_entryCount > 0;
}

} // namespace dsp
} // namespace mbootcore
