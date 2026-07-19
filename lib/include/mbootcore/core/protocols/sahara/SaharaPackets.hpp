#pragma once

#include "mbootcore/domain/IPacket.hpp"

namespace mbootcore {

class PacketHeader {
public:
    PacketHeader() = default;
    PacketHeader(uint32_t cmd, uint32_t len) : m_command(cmd), m_length(len) {}

    uint32_t command() const noexcept { return m_command; }
    uint32_t length() const noexcept { return m_length; }

protected:
    uint32_t m_command{0};
    uint32_t m_length{8};
};

class HelloPacket : public IPacket {
public:
    HelloPacket() = default;
    HelloPacket(uint32_t version, uint32_t versionSupported,
                uint32_t cmdPacketLength, uint32_t mode);

    uint32_t command() const noexcept override { return 0x01; }
    uint32_t length() const noexcept override { return 48; }

    uint32_t version() const noexcept { return m_version; }
    uint32_t versionSupported() const noexcept { return m_versionSupported; }
    uint32_t cmdPacketLength() const noexcept { return m_cmdPacketLength; }
    uint32_t mode() const noexcept { return m_mode; }

    std::string toString() const override;
    std::unique_ptr<IPacket> clone() const override;

private:
    uint32_t m_version{0};
    uint32_t m_versionSupported{0};
    uint32_t m_cmdPacketLength{0};
    uint32_t m_mode{0};
};

class HelloResponsePacket : public IPacket {
public:
    HelloResponsePacket() = default;
    HelloResponsePacket(uint32_t version, uint32_t versionSupported,
                        uint32_t status, uint32_t mode);

    uint32_t command() const noexcept override { return 0x02; }
    uint32_t length() const noexcept override { return 48; }

    uint32_t version() const noexcept { return m_version; }
    uint32_t versionSupported() const noexcept { return m_versionSupported; }
    uint32_t status() const noexcept { return m_status; }
    uint32_t mode() const noexcept { return m_mode; }

    std::string toString() const override;
    std::unique_ptr<IPacket> clone() const override;

private:
    uint32_t m_version{0};
    uint32_t m_versionSupported{0};
    uint32_t m_status{0};
    uint32_t m_mode{0};
};

class ReadDataPacket : public IPacket {
public:
    ReadDataPacket() = default;
    ReadDataPacket(uint32_t imageId, uint32_t dataOffset, uint32_t dataLength);

    uint32_t command() const noexcept override { return 0x03; }
    uint32_t length() const noexcept override { return 20; }

    uint32_t imageId() const noexcept { return m_imageId; }
    uint32_t dataOffset() const noexcept { return m_dataOffset; }
    uint32_t dataLength() const noexcept { return m_dataLength; }

    std::string toString() const override;
    std::unique_ptr<IPacket> clone() const override;

private:
    uint32_t m_imageId{0};
    uint32_t m_dataOffset{0};
    uint32_t m_dataLength{0};
};

class EndImageTransferPacket : public IPacket {
public:
    EndImageTransferPacket() = default;
    EndImageTransferPacket(uint32_t imageId, uint32_t status);

    uint32_t command() const noexcept override { return 0x04; }
    uint32_t length() const noexcept override { return 16; }

    uint32_t imageId() const noexcept { return m_imageId; }
    uint32_t status() const noexcept { return m_status; }

    std::string toString() const override;
    std::unique_ptr<IPacket> clone() const override;

private:
    uint32_t m_imageId{0};
    uint32_t m_status{0};
};

class DonePacket : public IPacket {
public:
    DonePacket() = default;

    uint32_t command() const noexcept override { return 0x05; }
    uint32_t length() const noexcept override { return 8; }

    std::string toString() const override;
    std::unique_ptr<IPacket> clone() const override;
};

class DoneResponsePacket : public IPacket {
public:
    DoneResponsePacket() = default;
    explicit DoneResponsePacket(uint32_t imageTxStatus);

    uint32_t command() const noexcept override { return 0x06; }
    uint32_t length() const noexcept override { return 12; }

    uint32_t imageTxStatus() const noexcept { return m_imageTxStatus; }

    std::string toString() const override;
    std::unique_ptr<IPacket> clone() const override;

private:
    uint32_t m_imageTxStatus{0};
};

class ResetPacket : public IPacket {
public:
    ResetPacket() = default;

    uint32_t command() const noexcept override { return 0x07; }
    uint32_t length() const noexcept override { return 8; }

    std::string toString() const override;
    std::unique_ptr<IPacket> clone() const override;
};

class ResetResponsePacket : public IPacket {
public:
    ResetResponsePacket() = default;

    uint32_t command() const noexcept override { return 0x08; }
    uint32_t length() const noexcept override { return 8; }

    std::string toString() const override;
    std::unique_ptr<IPacket> clone() const override;
};

class MemoryDebugPacket : public IPacket {
public:
    MemoryDebugPacket() = default;
    MemoryDebugPacket(uint32_t memTableAddr, uint32_t memTableLen);

    uint32_t command() const noexcept override { return 0x09; }
    uint32_t length() const noexcept override { return 16; }

    uint32_t memTableAddr() const noexcept { return m_memTableAddr; }
    uint32_t memTableLen() const noexcept { return m_memTableLen; }

    std::string toString() const override;
    std::unique_ptr<IPacket> clone() const override;

private:
    uint32_t m_memTableAddr{0};
    uint32_t m_memTableLen{0};
};

class ReadChipIdPacket : public IPacket {
public:
    ReadChipIdPacket() = default;
    ReadChipIdPacket(uint32_t chipIdLo, uint32_t chipIdHi,
                     uint32_t serialNum = 0, uint32_t msmId = 0,
                     uint32_t oemId = 0, uint32_t modelId = 0);

    uint32_t command() const noexcept override { return 0x0A; }
    uint32_t length() const noexcept override { return m_isV3 ? 40 : 16; }

    uint32_t chipIdLo() const noexcept { return m_chipIdLo; }
    uint32_t chipIdHi() const noexcept { return m_chipIdHi; }
    uint32_t serialNum() const noexcept { return m_serialNum; }
    uint32_t msmId() const noexcept { return m_msmId; }
    uint32_t oemId() const noexcept { return m_oemId; }
    uint32_t modelId() const noexcept { return m_modelId; }
    bool isV3() const noexcept { return m_isV3; }
    void setV3(bool v3) noexcept { m_isV3 = v3; }

    std::string toString() const override;
    std::unique_ptr<IPacket> clone() const override;

private:
    uint32_t m_chipIdLo{0};
    uint32_t m_chipIdHi{0};
    uint32_t m_serialNum{0};
    uint32_t m_msmId{0};
    uint32_t m_oemId{0};
    uint32_t m_modelId{0};
    bool m_isV3{false};
};

class CommandReadyPacket : public IPacket {
public:
    CommandReadyPacket() = default;

    uint32_t command() const noexcept override { return 0x0B; }
    uint32_t length() const noexcept override { return 8; }

    std::string toString() const override;
    std::unique_ptr<IPacket> clone() const override;
};

class CommandSwitchModePacket : public IPacket {
public:
    CommandSwitchModePacket() = default;
    explicit CommandSwitchModePacket(uint32_t mode);

    uint32_t command() const noexcept override { return 0x0C; }
    uint32_t length() const noexcept override { return 12; }

    uint32_t mode() const noexcept { return m_mode; }

    std::string toString() const override;
    std::unique_ptr<IPacket> clone() const override;

private:
    uint32_t m_mode{0};
};

class CommandExecPacket : public IPacket {
public:
    CommandExecPacket() = default;
    explicit CommandExecPacket(uint32_t clientCmd);

    uint32_t command() const noexcept override { return 0x0D; }
    uint32_t length() const noexcept override { return 12; }

    uint32_t clientCmd() const noexcept { return m_clientCmd; }

    std::string toString() const override;
    std::unique_ptr<IPacket> clone() const override;

private:
    uint32_t m_clientCmd{0};
};

class CommandExecResponsePacket : public IPacket {
public:
    CommandExecResponsePacket() = default;
    CommandExecResponsePacket(uint32_t clientCmd, uint32_t respLength);

    uint32_t command() const noexcept override { return 0x0E; }
    uint32_t length() const noexcept override { return 16; }

    uint32_t clientCmd() const noexcept { return m_clientCmd; }
    uint32_t respLength() const noexcept { return m_respLength; }

    std::string toString() const override;
    std::unique_ptr<IPacket> clone() const override;

private:
    uint32_t m_clientCmd{0};
    uint32_t m_respLength{0};
};

class ExecuteDataPacket : public IPacket {
public:
    ExecuteDataPacket() = default;
    ExecuteDataPacket(uint32_t clientCmd, uint32_t dataLen);

    uint32_t command() const noexcept override { return 0x0F; }
    uint32_t length() const noexcept override { return 16; }

    uint32_t clientCmd() const noexcept { return m_clientCmd; }
    uint32_t dataLength() const noexcept { return m_dataLength; }

    std::string toString() const override;
    std::unique_ptr<IPacket> clone() const override;

private:
    uint32_t m_clientCmd{0};
    uint32_t m_dataLength{0};
};

class ExecuteDataResponsePacket : public IPacket {
public:
    ExecuteDataResponsePacket() = default;
    ExecuteDataResponsePacket(uint32_t clientCmd, uint32_t status);

    uint32_t command() const noexcept override { return 0x10; }
    uint32_t length() const noexcept override { return 16; }

    uint32_t clientCmd() const noexcept { return m_clientCmd; }
    uint32_t status() const noexcept { return m_status; }

    std::string toString() const override;
    std::unique_ptr<IPacket> clone() const override;

private:
    uint32_t m_clientCmd{0};
    uint32_t m_status{0};
};

class RxDataPacket : public IPacket {
public:
    RxDataPacket() = default;
    RxDataPacket(uint32_t imageId, uint32_t dataOffset, uint32_t dataLength);

    uint32_t command() const noexcept override { return 0x11; }
    uint32_t length() const noexcept override { return 20; }

    uint32_t imageId() const noexcept { return m_imageId; }
    uint32_t dataOffset() const noexcept { return m_dataOffset; }
    uint32_t dataLength() const noexcept { return m_dataLength; }

    std::string toString() const override;
    std::unique_ptr<IPacket> clone() const override;

private:
    uint32_t m_imageId{0};
    uint32_t m_dataOffset{0};
    uint32_t m_dataLength{0};
};

class RxDataResponsePacket : public IPacket {
public:
    RxDataResponsePacket() = default;
    RxDataResponsePacket(uint32_t imageId, uint32_t status);

    uint32_t command() const noexcept override { return 0x12; }
    uint32_t length() const noexcept override { return 16; }

    uint32_t imageId() const noexcept { return m_imageId; }
    uint32_t status() const noexcept { return m_status; }

    std::string toString() const override;
    std::unique_ptr<IPacket> clone() const override;

private:
    uint32_t m_imageId{0};
    uint32_t m_status{0};
};

class ResetStateMachinePacket : public IPacket {
public:
    ResetStateMachinePacket() = default;

    uint32_t command() const noexcept override { return 0x13; }
    uint32_t length() const noexcept override { return 8; }

    std::string toString() const override;
    std::unique_ptr<IPacket> clone() const override;
};

} // namespace mbootcore
