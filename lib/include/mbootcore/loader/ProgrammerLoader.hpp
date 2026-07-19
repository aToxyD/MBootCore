#pragma once

#include "mbootcore/domain/ILoader.hpp"

#include <string>

namespace mbootcore {

class ProgrammerLoader : public ILoader {
public:
    explicit ProgrammerLoader(ByteBuffer data);

    const LoaderInfo& info() const noexcept override { return m_info; }
    const ByteBuffer& data() const noexcept override { return m_data; }
    Result<bool> isCompatibleWith(const DeviceId& deviceId) const override;
    Result<void> parse() override;

private:
    LoaderInfo m_info;
    ByteBuffer m_data;
    bool m_parsed{false};
};

} // namespace mbootcore
