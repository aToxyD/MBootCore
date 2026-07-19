#include "mbootcore/loader/ProgrammerLoader.hpp"
#include "mbootcore/loader/ElfInspector.hpp"
#include <algorithm>
#include <cstring>

namespace mbootcore {

ProgrammerLoader::ProgrammerLoader(ByteBuffer data)
    : m_data(std::move(data)) {}

Result<bool> ProgrammerLoader::isCompatibleWith(const DeviceId& deviceId) const {
    const auto& compat = m_info.compatibleId;
    if (compat.pkhash != std::array<uint8_t, 32>{}) {
        bool hashMatch = deviceId.pkhash == compat.pkhash;
        return hashMatch;
    }
    if (compat.msmId != 0 && compat.msmId != deviceId.msmId) {
        return false;
    }
    if (compat.oemId != 0 && compat.oemId != deviceId.oemId) {
        return false;
    }
    if (compat.modelId != 0 && compat.modelId != deviceId.modelId) {
        return false;
    }
    if (compat.msmId == 0 && compat.oemId == 0 && compat.modelId == 0) {
        return false;
    }
    return true;
}

Result<void> ProgrammerLoader::parse() {
    if (m_parsed) {
        return {};
    }
    ElfInspector inspector;
    if (!inspector.isElf(m_data)) {
        return ErrorCode::InvalidElf;
    }
    auto result = inspector.inspect(m_data);
    if (result.isError()) {
        return result.error();
    }
    auto& info = result.value();
    m_info.filename = "programmer";
    m_info.targetDevice = info.architecture;
    m_info.imageType = info.header.type;
    m_parsed = true;
    return {};
}

} // namespace mbootcore
