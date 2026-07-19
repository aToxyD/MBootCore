#pragma once

#include <mbootcore/transport/serial/ISerialBackend.hpp>

namespace mbootcore {
namespace transport {
namespace serial {

class PosixSerialBackend : public ISerialBackend {
public:
    explicit PosixSerialBackend(ILogger* logger = nullptr);
    ~PosixSerialBackend() override;

    bool isAvailable() const noexcept override;

    Result<void> open(const std::string& portName, int baudRate,
                       int dataBits, int stopBits,
                       const std::string& parity,
                       const std::string& flowControl,
                       size_t readBufferSize) override;
    void close() noexcept override;
    bool isOpen() const noexcept override;

    Result<size_t> write(const uint8_t* data, size_t size,
                          std::chrono::milliseconds timeout) override;
    Result<size_t> read(uint8_t* buffer, size_t size,
                         std::chrono::milliseconds timeout) override;

    void cancel() noexcept override;
    Result<void> flush() override;

    std::string backendName() const noexcept override { return "PosixTermios"; }
    void setLogger(ILogger* logger) noexcept override { m_logger = logger; }

private:
    int m_fd{-1};
    ILogger* m_logger;
    bool m_cancelled{false};
};

} // namespace serial
} // namespace transport
} // namespace mbootcore
