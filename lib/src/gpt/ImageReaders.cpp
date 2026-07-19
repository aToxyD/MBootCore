#include "mbootcore/gpt/ImageReaders.hpp"
#include <fstream>
#include <vector>

namespace mbootcore {
namespace gpt {

// ── RawImageReader ──

Result<void> RawImageReader::open(const std::string& path) {
    (void)close();
    std::ifstream ifs(path, std::ios::binary | std::ios::ate);
    if (!ifs.is_open()) {
        return ErrorCode::InvalidArgument;
    }
    m_size = static_cast<uint64_t>(ifs.tellg());
    m_path = path;
    m_open = true;
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
    std::ifstream ifs(m_path, std::ios::binary);
    if (!ifs.is_open()) {
        return ErrorCode::InvalidArgument;
    }
    ifs.seekg(static_cast<std::streamoff>(offset));
    if (ifs.fail()) {
        return ErrorCode::InvalidArgument;
    }
    ByteBuffer buf(size);
    ifs.read(reinterpret_cast<char*>(buf.data()), static_cast<std::streamsize>(size));
    auto bytesRead = ifs.gcount();
    if (bytesRead < static_cast<std::streamsize>(size)) {
        buf.resize(static_cast<size_t>(bytesRead));
    }
    return buf;
}

Result<ByteBuffer> RawImageReader::readAll() {
    return read(0, static_cast<size_t>(m_size));
}

// ── BinaryImageReader ──

Result<void> BinaryImageReader::open(const std::string& path) {
    (void)close();
    std::ifstream ifs(path, std::ios::binary | std::ios::ate);
    if (!ifs.is_open()) {
        return ErrorCode::InvalidArgument;
    }
    m_size = static_cast<uint64_t>(ifs.tellg());
    m_path = path;
    m_open = true;
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
    std::ifstream ifs(m_path, std::ios::binary);
    if (!ifs.is_open()) {
        return ErrorCode::InvalidArgument;
    }
    ifs.seekg(static_cast<std::streamoff>(offset));
    if (ifs.fail()) {
        return ErrorCode::InvalidArgument;
    }
    ByteBuffer buf(size);
    ifs.read(reinterpret_cast<char*>(buf.data()), static_cast<std::streamsize>(size));
    auto bytesRead = ifs.gcount();
    if (bytesRead < static_cast<std::streamsize>(size)) {
        buf.resize(static_cast<size_t>(bytesRead));
    }
    return buf;
}

Result<ByteBuffer> BinaryImageReader::readAll() {
    return read(0, static_cast<size_t>(m_size));
}

}} // namespace mbootcore::gpt
