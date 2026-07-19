#pragma once

#include "mbootcore/loader/ILoaderData.hpp"

#include <utility>

namespace mbootcore {

class LoaderData : public ILoaderData {
public:
    LoaderData(std::string name, ByteBuffer data, LoaderMetadata metadata)
        : m_name(std::move(name))
        , m_data(std::move(data))
        , m_metadata(std::move(metadata)) {}

    const ByteBuffer& data() const noexcept override { return m_data; }
    const LoaderMetadata& metadata() const noexcept override { return m_metadata; }
    std::string_view name() const noexcept override { return m_name; }

private:
    std::string m_name;
    ByteBuffer m_data;
    LoaderMetadata m_metadata;
};

} // namespace mbootcore
