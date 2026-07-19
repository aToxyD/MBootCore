#include <mbootcore/security/SecurityTypes.hpp>
#include <sstream>
#include <iomanip>

namespace mbootcore { namespace security {

std::string SecureHash::toString() const {
    if (value.empty()) return "(empty)";
    return value;
}

bool SecureHash::operator==(const SecureHash& o) const noexcept {
    return algorithm == o.algorithm && value == o.value;
}

bool SecureHash::operator!=(const SecureHash& o) const noexcept {
    return !(*this == o);
}

} }
