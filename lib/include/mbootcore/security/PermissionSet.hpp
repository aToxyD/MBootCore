#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <map>

namespace mbootcore { namespace security {

enum class Permission : uint32_t {
    Read=0, Write=1, Execute=2, Install=3, Remove=4,
    Configure=5, Debug=6, Admin=7
};

class PermissionSet {
public:
    PermissionSet();
    explicit PermissionSet(const std::vector<Permission>& permissions);

    void grant(Permission perm);
    void revoke(Permission perm);
    bool has(Permission perm) const;
    bool hasAll(const std::vector<Permission>& permissions) const;
    bool hasAny(const std::vector<Permission>& permissions) const;
    void clear();
    std::vector<Permission> permissions() const;
    size_t count() const;

    bool operator==(const PermissionSet& o) const noexcept;
    bool operator!=(const PermissionSet& o) const noexcept;

private:
    std::map<Permission, bool> m_perms;
};

} }
