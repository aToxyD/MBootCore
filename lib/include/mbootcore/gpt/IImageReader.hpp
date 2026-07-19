#pragma once

#include "mbootcore/domain/Types.hpp"
#include "mbootcore/domain/Error.hpp"

#include <string>
#include <string_view>

namespace mbootcore {
namespace gpt {

class IImageReader {
public:
    virtual ~IImageReader() = default;

    virtual Result<void> open(const std::string& path) = 0;
    virtual Result<void> close() = 0;
    virtual bool isOpen() const noexcept = 0;
    virtual uint64_t size() const noexcept = 0;
    virtual std::string_view format() const noexcept = 0;

    virtual Result<ByteBuffer> read(uint64_t offset, size_t size) = 0;
    virtual Result<ByteBuffer> readAll() = 0;
};

}} // namespace mbootcore::gpt
