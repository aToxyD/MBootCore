#include <mbootcore/security/PermissionSet.hpp>
#include <algorithm>

namespace mbootcore { namespace security {

PermissionSet::PermissionSet() {
    for (uint32_t i = 0; i <= static_cast<uint32_t>(Permission::Admin); ++i) {
        m_perms[static_cast<Permission>(i)] = false;
    }
}

PermissionSet::PermissionSet(const std::vector<Permission>& permissions) {
    for (uint32_t i = 0; i <= static_cast<uint32_t>(Permission::Admin); ++i) {
        m_perms[static_cast<Permission>(i)] = false;
    }
    for (auto p : permissions) {
        m_perms[p] = true;
    }
}

void PermissionSet::grant(Permission perm) {
    m_perms[perm] = true;
}

void PermissionSet::revoke(Permission perm) {
    m_perms[perm] = false;
}

bool PermissionSet::has(Permission perm) const {
    auto it = m_perms.find(perm);
    if (it == m_perms.end()) return false;
    return it->second;
}

bool PermissionSet::hasAll(const std::vector<Permission>& permissions) const {
    for (auto p : permissions) {
        if (!has(p)) return false;
    }
    return true;
}

bool PermissionSet::hasAny(const std::vector<Permission>& permissions) const {
    for (auto p : permissions) {
        if (has(p)) return true;
    }
    return false;
}

void PermissionSet::clear() {
    for (auto& kv : m_perms) {
        kv.second = false;
    }
}

std::vector<Permission> PermissionSet::permissions() const {
    std::vector<Permission> result;
    for (auto& kv : m_perms) {
        if (kv.second) {
            result.push_back(kv.first);
        }
    }
    return result;
}

size_t PermissionSet::count() const {
    size_t c = 0;
    for (auto& kv : m_perms) {
        if (kv.second) ++c;
    }
    return c;
}

bool PermissionSet::operator==(const PermissionSet& o) const noexcept {
    return m_perms == o.m_perms;
}

bool PermissionSet::operator!=(const PermissionSet& o) const noexcept {
    return m_perms != o.m_perms;
}

} }
