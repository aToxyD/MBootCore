#include <mbootcore/dsp/LoaderRepository.hpp>
#include <mbootcore/dsp/DSPRepository.hpp>
#include <mbootcore/dsp/DSPCache.hpp>
#include <fstream>
#include <filesystem>
#include <algorithm>
#include <set>

namespace fs = std::filesystem;
using namespace mbootcore::discovery;

namespace mbootcore {
namespace dsp {

LoaderIndex::LoaderIndex(const DSPRepository& repo)
    : m_repo(repo) {}

void LoaderIndex::refresh() {
    m_index = m_repo.allLoaders();
}

size_t LoaderIndex::count() const noexcept {
    return m_index.size();
}

std::vector<std::string> LoaderIndex::allLoaderIds() const {
    std::vector<std::string> ids;
    ids.reserve(m_index.size());
    for (auto* ldr : m_index) {
        ids.push_back(ldr->loaderId);
    }
    return ids;
}

const DSPLoaderMetadata* LoaderIndex::byId(const std::string& loaderId) const {
    for (auto* ldr : m_index) {
        if (ldr->loaderId == loaderId) return ldr;
    }
    return nullptr;
}

std::vector<const DSPLoaderMetadata*> LoaderIndex::byChipset(const ChipsetId& id) const {
    std::vector<const DSPLoaderMetadata*> result;
    for (auto* ldr : m_index) {
        for (const auto& cid : ldr->compatibleChipsets) {
            if (cid.vendor == id.vendor && cid.family == id.family &&
                (id.variant.empty() || cid.variant == id.variant)) {
                result.push_back(ldr);
                break;
            }
        }
    }
    return result;
}

std::vector<const DSPLoaderMetadata*> LoaderIndex::byProtocol(ProtocolType proto) const {
    std::vector<const DSPLoaderMetadata*> result;
    for (auto* ldr : m_index) {
        for (auto p : ldr->protocols) {
            if (p == proto) { result.push_back(ldr); break; }
        }
    }
    return result;
}

std::vector<const DSPLoaderMetadata*> LoaderIndex::byVendor(Vendor vendor) const {
    std::vector<const DSPLoaderMetadata*> result;
    if (vendor == Vendor::Unknown) return result;
    auto vendorStr = std::to_string(static_cast<int>(vendor));
    for (auto* ldr : m_index) {
        for (const auto& v : ldr->compatibleVendors) {
            if (v == vendorStr) { result.push_back(ldr); break; }
        }
    }
    return result;
}

std::vector<const DSPLoaderMetadata*> LoaderIndex::search(const std::string& query) const {
    std::vector<const DSPLoaderMetadata*> result;
    auto lower = query;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    for (auto* ldr : m_index) {
        auto id = ldr->loaderId;
        std::transform(id.begin(), id.end(), id.begin(), ::tolower);
        if (id.find(lower) != std::string::npos) {
            result.push_back(ldr);
        }
    }
    return result;
}

LoaderDatabase::LoaderDatabase(const DSPRepository& repo)
    : m_repo(repo) {}

Result<void> LoaderDatabase::rebuild() {
    m_entries = m_repo.allLoaders();
    return {};
}

const DSPLoaderMetadata* LoaderDatabase::findById(const std::string& loaderId) const {
    for (auto* e : m_entries) {
        if (e->loaderId == loaderId) return e;
    }
    return nullptr;
}

std::vector<const DSPLoaderMetadata*> LoaderDatabase::findByChipset(const ChipsetId& id) const {
    std::vector<const DSPLoaderMetadata*> result;
    for (auto* ldr : m_entries) {
        for (const auto& cid : ldr->compatibleChipsets) {
            if (cid.vendor == id.vendor && cid.family == id.family &&
                (id.variant.empty() || cid.variant == id.variant)) {
                result.push_back(ldr);
                break;
            }
        }
    }
    return result;
}

std::vector<const DSPLoaderMetadata*> LoaderDatabase::findByProtocol(ProtocolType proto) const {
    std::vector<const DSPLoaderMetadata*> result;
    for (auto* ldr : m_entries) {
        for (auto p : ldr->protocols) {
            if (p == proto) { result.push_back(ldr); break; }
        }
    }
    return result;
}

std::vector<const DSPLoaderMetadata*> LoaderDatabase::findByVendor(Vendor vendor) const {
    std::vector<const DSPLoaderMetadata*> result;
    if (vendor == Vendor::Unknown) return result;
    auto vendorStr = std::to_string(static_cast<int>(vendor));
    for (auto* ldr : m_entries) {
        for (const auto& v : ldr->compatibleVendors) {
            if (v == vendorStr) { result.push_back(ldr); break; }
        }
    }
    return result;
}

std::vector<const DSPLoaderMetadata*> LoaderDatabase::findByBootMode(BootMode mode) const {
    std::vector<const DSPLoaderMetadata*> result;
    for (auto* ldr : m_entries) {
        for (auto bm : ldr->bootModes) {
            if (bm == mode) { result.push_back(ldr); break; }
        }
    }
    return result;
}

std::vector<const DSPLoaderMetadata*> LoaderDatabase::findWithTag(const std::string& tag) const {
    std::vector<const DSPLoaderMetadata*> result;
    for (auto* ldr : m_entries) {
        for (const auto& t : ldr->tags) {
            if (t == tag) { result.push_back(ldr); break; }
        }
    }
    return result;
}

size_t LoaderDatabase::count() const noexcept {
    return m_entries.size();
}

LoaderMatcher::LoaderMatcher(const LoaderDatabase& db)
    : m_db(db) {}

std::vector<LoaderMatcher::MatchResult> LoaderMatcher::findBest(
    const ChipsetId& chipset, ProtocolType protocol, BootMode mode) const {
    std::vector<MatchResult> results;
    auto candidates = m_db.findByChipset(chipset);
    if (candidates.empty()) {
        candidates = m_db.findByVendor(Vendor::Unknown);
    }
    for (auto* ldr : candidates) {
        MatchResult mr;
        mr.loader = ldr;
        mr.score = scoreLoader(*ldr, chipset, protocol, mode);
        results.push_back(mr);
    }
    std::sort(results.begin(), results.end(),
        [](const MatchResult& a, const MatchResult& b) { return a.score > b.score; });
    return results;
}

const DSPLoaderMetadata* LoaderMatcher::findBestSingle(
    const ChipsetId& chipset, ProtocolType protocol, BootMode mode) const {
    auto results = findBest(chipset, protocol, mode);
    return results.empty() ? nullptr : results[0].loader;
}

int LoaderMatcher::scoreLoader(const DSPLoaderMetadata& loader, const ChipsetId& chipset,
    ProtocolType protocol, BootMode mode) const {
    int score = 0;
    for (const auto& cid : loader.compatibleChipsets) {
        if (cid.vendor == chipset.vendor && cid.family == chipset.family) {
            score += 50;
            if (!chipset.variant.empty() && cid.variant == chipset.variant) score += 30;
        }
    }
    for (auto p : loader.protocols) {
        if (p == protocol) score += 20;
    }
    for (auto bm : loader.bootModes) {
        if (bm == mode) score += 10;
    }
    if (loader.isPreferred) score += 15;
    score += loader.priority;
    return score;
}

LoaderResolver::LoaderResolver(const LoaderDatabase& db, const LoaderMatcher& matcher)
    : m_db(db), m_matcher(matcher) {}

LoaderResolver::ResolvedLoader LoaderResolver::resolve(
    const ChipsetId& chipset, ProtocolType protocol, BootMode mode) const {
    ResolvedLoader result;
    auto matches = m_matcher.findBest(chipset, protocol, mode);
    if (!matches.empty()) {
        result.primary = matches[0].loader;
        for (size_t i = 1; i < matches.size(); ++i) {
            result.fallbacks.push_back(matches[i].loader);
        }
    }
    return result;
}

// ============================================================================
// LoaderCache (PIMPL)
// ============================================================================
struct LoaderCache::Impl {
    std::string cachePath;
    std::set<std::string> m_cached;

    std::string loaderPath(const std::string& id) const {
        return cachePath + "/" + id + ".ldr";
    }
};

LoaderCache::LoaderCache(const std::string& cachePath)
    : m_impl(std::make_unique<Impl>()) {
    m_impl->cachePath = cachePath;
    fs::create_directories(cachePath);
    if (fs::is_directory(cachePath)) {
        for (auto& entry : fs::directory_iterator(cachePath)) {
            if (entry.path().extension() == ".ldr") {
                m_impl->m_cached.insert(entry.path().stem().string());
            }
        }
    }
}

LoaderCache::~LoaderCache() = default;

Result<void> LoaderCache::cacheLoader(const std::string& loaderId, const ByteBuffer& data) {
    auto path = m_impl->loaderPath(loaderId);
    std::ofstream out(path, std::ios::binary);
    if (!out) return ErrorCode::Unknown;
    out.write(reinterpret_cast<const char*>(data.data()), data.size());
    if (!out) return ErrorCode::Unknown;
    m_impl->m_cached.insert(loaderId);
    return {};
}

Result<ByteBuffer> LoaderCache::loadCached(const std::string& loaderId) const {
    auto path = m_impl->loaderPath(loaderId);
    std::ifstream in(path, std::ios::binary | std::ios::ate);
    if (!in) return ErrorCode::CacheMiss;
    auto size = in.tellg();
    in.seekg(0);
    ByteBuffer data(static_cast<size_t>(size));
    if (!in.read(reinterpret_cast<char*>(data.data()), size)) {
        return ErrorCode::Unknown;
    }
    return data;
}

bool LoaderCache::isCached(const std::string& loaderId) const noexcept {
    return m_impl->m_cached.find(loaderId) != m_impl->m_cached.end();
}

Result<void> LoaderCache::invalidate(const std::string& loaderId) {
    auto path = m_impl->loaderPath(loaderId);
    fs::remove(path);
    m_impl->m_cached.erase(loaderId);
    return {};
}

Result<void> LoaderCache::clear() {
    for (const auto& id : m_impl->m_cached) {
        fs::remove(m_impl->loaderPath(id));
    }
    m_impl->m_cached.clear();
    return {};
}

size_t LoaderCache::cachedCount() const noexcept {
    return m_impl->m_cached.size();
}

uint64_t LoaderCache::totalCachedSize() const noexcept {
    uint64_t total = 0;
    for (const auto& id : m_impl->m_cached) {
        auto path = m_impl->loaderPath(id);
        try { total += fs::file_size(path); } catch (...) {}
    }
    return total;
}

Result<bool> LoaderVerifier::verifyIntegrity(const DSPLoaderMetadata&, const ByteBuffer&) const {
    return true;
}

Result<bool> LoaderVerifier::verifyHash(const DSPLoaderMetadata&, const ByteBuffer&) const {
    return true;
}

Result<bool> LoaderVerifier::verifySignature(const DSPLoaderMetadata&, const ByteBuffer&) const {
    return true;
}

Result<bool> LoaderVerifier::verifyCompatibility(const DSPLoaderMetadata&) const {
    return true;
}

} // namespace dsp
} // namespace mbootcore
