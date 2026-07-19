#pragma once

#include "mbootcore/gpt/IImageReader.hpp"

#include <string>

namespace mbootcore {
namespace gpt {

class RawImageReader : public IImageReader {
public:
    RawImageReader() = default;

    Result<void> open(const std::string& path) override;
    Result<void> close() override;
    bool isOpen() const noexcept override;
    uint64_t size() const noexcept override;
    std::string_view format() const noexcept override;

    Result<ByteBuffer> read(uint64_t offset, size_t size) override;
    Result<ByteBuffer> readAll() override;

private:
    std::string m_path;
    uint64_t m_size{0};
    bool m_open{false};
};

class BinaryImageReader : public IImageReader {
public:
    BinaryImageReader() = default;

    Result<void> open(const std::string& path) override;
    Result<void> close() override;
    bool isOpen() const noexcept override;
    uint64_t size() const noexcept override;
    std::string_view format() const noexcept override;

    Result<ByteBuffer> read(uint64_t offset, size_t size) override;
    Result<ByteBuffer> readAll() override;

private:
    std::string m_path;
    uint64_t m_size{0};
    bool m_open{false};
};

}} // namespace mbootcore::gpt
