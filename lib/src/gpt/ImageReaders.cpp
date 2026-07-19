#include "mbootcore/gpt/ImageReaders.hpp"
#include <cstdio>
#include <vector>

namespace mbootcore {
namespace gpt {

// ── RawImageReader ──

Result<void> RawImageReader::open(const std::string& path) {
    (void)close();
    FILE* f = std::fopen(path.c_str(), "rb");
    if (!f) {
        return ErrorCode::InvalidArgument;
    }
    std::fseek(f, 0, SEEK_END);
    long sz = std::ftell(f);
    if (sz < 0) {
        std::fclose(f);
        return ErrorCode::InvalidArgument;
    }
    m_size = static_cast<uint64_t>(sz);
    m_path = path;
    m_open = true;
    std::fclose(f);
    return {};
}

Result<void> RawImageReader::close() {
    m_open = false;
    m_path.clear();
    m_size = 0;
    return {};
}

bool RawImageReader::isOpen() const noexcept {
    return m_open;
}

uint64_t RawImageReader::size() const noexcept {
    return m_size;
}

std::string_view RawImageReader::format() const noexcept {
    return "raw";
}

Result<ByteBuffer> RawImageReader::read(uint64_t offset, size_t size) {
    if (!m_open) {
        return ErrorCode::InvalidArgument;
    }
    FILE* f = std::fopen(m_path.c_str(), "rb");
    if (!f) {
        return ErrorCode::InvalidArgument;
    }
    if (std::fseek(f, static_cast<long>(offset), SEEK_SET) != 0) {
        std::fclose(f);
        return ErrorCode::InvalidArgument;
    }
    ByteBuffer buf(size);
    size_t readBytes = std::fread(buf.data(), 1, size, f);
    std::fclose(f);
    if (readBytes < size) {
        buf.resize(readBytes);
    }
    return buf;
}

Result<ByteBuffer> RawImageReader::readAll() {
    return read(0, static_cast<size_t>(m_size));
}

// ── BinaryImageReader ──

Result<void> BinaryImageReader::open(const std::string& path) {
    (void)close();
    FILE* f = std::fopen(path.c_str(), "rb");
    if (!f) {
        return ErrorCode::InvalidArgument;
    }
    std::fseek(f, 0, SEEK_END);
    long sz = std::ftell(f);
    if (sz < 0) {
        std::fclose(f);
        return ErrorCode::InvalidArgument;
    }
    m_size = static_cast<uint64_t>(sz);
    m_path = path;
    m_open = true;
    std::fclose(f);
    return {};
}

Result<void> BinaryImageReader::close() {
    m_open = false;
    m_path.clear();
    m_size = 0;
    return {};
}

bool BinaryImageReader::isOpen() const noexcept {
    return m_open;
}

uint64_t BinaryImageReader::size() const noexcept {
    return m_size;
}

std::string_view BinaryImageReader::format() const noexcept {
    return "binary";
}

Result<ByteBuffer> BinaryImageReader::read(uint64_t offset, size_t size) {
    if (!m_open) {
        return ErrorCode::InvalidArgument;
    }
    FILE* f = std::fopen(m_path.c_str(), "rb");
    if (!f) {
        return ErrorCode::InvalidArgument;
    }
    if (std::fseek(f, static_cast<long>(offset), SEEK_SET) != 0) {
        std::fclose(f);
        return ErrorCode::InvalidArgument;
    }
    ByteBuffer buf(size);
    size_t readBytes = std::fread(buf.data(), 1, size, f);
    std::fclose(f);
    if (readBytes < size) buf.resize(readBytes);
    return buf;
}

Result<ByteBuffer> BinaryImageReader::readAll() {
    return read(0, static_cast<size_t>(m_size));
}

}} // namespace mbootcore::gpt
