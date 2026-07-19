#include "mbootcore/core/protocols/sahara/SaharaPackets.hpp"

#include <sstream>
#include <iomanip>

namespace mbootcore {

namespace {

std::string toHex(uint32_t val) {
    std::ostringstream oss;
    oss << "0x" << std::hex << std::uppercase << val;
    return oss.str();
}

} // namespace

HelloPacket::HelloPacket(uint32_t version, uint32_t versionSupported,
                         uint32_t cmdPacketLength, uint32_t mode)
    : m_version(version)
    , m_versionSupported(versionSupported)
    , m_cmdPacketLength(cmdPacketLength)
    , m_mode(mode) {}

std::string HelloPacket::toString() const {
    std::ostringstream oss;
    oss << "HelloPacket(ver=" << m_version
        << ", ver_sup=" << m_versionSupported
        << ", cmd_len=" << m_cmdPacketLength
        << ", mode=" << toHex(m_mode) << ")";
    return oss.str();
}

std::unique_ptr<IPacket> HelloPacket::clone() const {
    return std::make_unique<HelloPacket>(*this);
}

HelloResponsePacket::HelloResponsePacket(uint32_t version, uint32_t versionSupported,
                                         uint32_t status, uint32_t mode)
    : m_version(version)
    , m_versionSupported(versionSupported)
    , m_status(status)
    , m_mode(mode) {}

std::string HelloResponsePacket::toString() const {
    std::ostringstream oss;
    oss << "HelloResponsePacket(ver=" << m_version
        << ", ver_sup=" << m_versionSupported
        << ", status=" << toHex(m_status)
        << ", mode=" << toHex(m_mode) << ")";
    return oss.str();
}

std::unique_ptr<IPacket> HelloResponsePacket::clone() const {
    return std::make_unique<HelloResponsePacket>(*this);
}

ReadDataPacket::ReadDataPacket(uint32_t imageId, uint32_t dataOffset, uint32_t dataLength)
    : m_imageId(imageId)
    , m_dataOffset(dataOffset)
    , m_dataLength(dataLength) {}

std::string ReadDataPacket::toString() const {
    std::ostringstream oss;
    oss << "ReadDataPacket(img=" << m_imageId
        << ", offset=" << toHex(m_dataOffset)
        << ", len=" << m_dataLength << ")";
    return oss.str();
}

std::unique_ptr<IPacket> ReadDataPacket::clone() const {
    return std::make_unique<ReadDataPacket>(*this);
}

EndImageTransferPacket::EndImageTransferPacket(uint32_t imageId, uint32_t status)
    : m_imageId(imageId)
    , m_status(status) {}

std::string EndImageTransferPacket::toString() const {
    std::ostringstream oss;
    oss << "EndImageTransferPacket(img=" << m_imageId
        << ", status=" << toHex(m_status) << ")";
    return oss.str();
}

std::unique_ptr<IPacket> EndImageTransferPacket::clone() const {
    return std::make_unique<EndImageTransferPacket>(*this);
}

std::string DonePacket::toString() const {
    return "DonePacket";
}

std::unique_ptr<IPacket> DonePacket::clone() const {
    return std::make_unique<DonePacket>(*this);
}

DoneResponsePacket::DoneResponsePacket(uint32_t imageTxStatus)
    : m_imageTxStatus(imageTxStatus) {}

std::string DoneResponsePacket::toString() const {
    std::ostringstream oss;
    oss << "DoneResponsePacket(status=" << m_imageTxStatus << ")";
    return oss.str();
}

std::unique_ptr<IPacket> DoneResponsePacket::clone() const {
    return std::make_unique<DoneResponsePacket>(*this);
}

std::string ResetPacket::toString() const {
    return "ResetPacket";
}

std::unique_ptr<IPacket> ResetPacket::clone() const {
    return std::make_unique<ResetPacket>(*this);
}

std::string ResetResponsePacket::toString() const {
    return "ResetResponsePacket";
}

std::unique_ptr<IPacket> ResetResponsePacket::clone() const {
    return std::make_unique<ResetResponsePacket>(*this);
}

MemoryDebugPacket::MemoryDebugPacket(uint32_t memTableAddr, uint32_t memTableLen)
    : m_memTableAddr(memTableAddr)
    , m_memTableLen(memTableLen) {}

std::string MemoryDebugPacket::toString() const {
    std::ostringstream oss;
    oss << "MemoryDebugPacket(addr=" << toHex(m_memTableAddr)
        << ", len=" << m_memTableLen << ")";
    return oss.str();
}

std::unique_ptr<IPacket> MemoryDebugPacket::clone() const {
    return std::make_unique<MemoryDebugPacket>(*this);
}

ReadChipIdPacket::ReadChipIdPacket(uint32_t chipIdLo, uint32_t chipIdHi,
                                     uint32_t serialNum, uint32_t msmId,
                                     uint32_t oemId, uint32_t modelId)
    : m_chipIdLo(chipIdLo)
    , m_chipIdHi(chipIdHi)
    , m_serialNum(serialNum)
    , m_msmId(msmId)
    , m_oemId(oemId)
    , m_modelId(modelId) {
    m_isV3 = (serialNum != 0 || msmId != 0 || oemId != 0 || modelId != 0);
}

std::string ReadChipIdPacket::toString() const {
    std::ostringstream oss;
    oss << "ReadChipIdPacket(chip_id=" << toHex(m_chipIdLo) << toHex(m_chipIdHi);
    if (m_isV3) {
        oss << ", serial=" << m_serialNum
            << ", msm_id=" << toHex(m_msmId)
            << ", oem_id=" << toHex(m_oemId)
            << ", model_id=" << m_modelId;
    }
    oss << ")";
    return oss.str();
}

std::unique_ptr<IPacket> ReadChipIdPacket::clone() const {
    return std::make_unique<ReadChipIdPacket>(*this);
}

std::string CommandReadyPacket::toString() const {
    return "CommandReadyPacket";
}

std::unique_ptr<IPacket> CommandReadyPacket::clone() const {
    return std::make_unique<CommandReadyPacket>(*this);
}

CommandSwitchModePacket::CommandSwitchModePacket(uint32_t mode)
    : m_mode(mode) {}

std::string CommandSwitchModePacket::toString() const {
    std::ostringstream oss;
    oss << "CommandSwitchModePacket(mode=" << toHex(m_mode) << ")";
    return oss.str();
}

std::unique_ptr<IPacket> CommandSwitchModePacket::clone() const {
    return std::make_unique<CommandSwitchModePacket>(*this);
}

CommandExecPacket::CommandExecPacket(uint32_t clientCmd)
    : m_clientCmd(clientCmd) {}

std::string CommandExecPacket::toString() const {
    std::ostringstream oss;
    oss << "CommandExecPacket(cmd=" << toHex(m_clientCmd) << ")";
    return oss.str();
}

std::unique_ptr<IPacket> CommandExecPacket::clone() const {
    return std::make_unique<CommandExecPacket>(*this);
}

CommandExecResponsePacket::CommandExecResponsePacket(uint32_t clientCmd, uint32_t respLength)
    : m_clientCmd(clientCmd)
    , m_respLength(respLength) {}

std::string CommandExecResponsePacket::toString() const {
    std::ostringstream oss;
    oss << "CommandExecResponsePacket(cmd=" << toHex(m_clientCmd)
        << ", resp_len=" << m_respLength << ")";
    return oss.str();
}

std::unique_ptr<IPacket> CommandExecResponsePacket::clone() const {
    return std::make_unique<CommandExecResponsePacket>(*this);
}

ExecuteDataPacket::ExecuteDataPacket(uint32_t clientCmd, uint32_t dataLen)
    : m_clientCmd(clientCmd)
    , m_dataLength(dataLen) {}

std::string ExecuteDataPacket::toString() const {
    std::ostringstream oss;
    oss << "ExecuteDataPacket(cmd=" << toHex(m_clientCmd)
        << ", data_len=" << m_dataLength << ")";
    return oss.str();
}

std::unique_ptr<IPacket> ExecuteDataPacket::clone() const {
    return std::make_unique<ExecuteDataPacket>(*this);
}

ExecuteDataResponsePacket::ExecuteDataResponsePacket(uint32_t clientCmd, uint32_t status)
    : m_clientCmd(clientCmd)
    , m_status(status) {}

std::string ExecuteDataResponsePacket::toString() const {
    std::ostringstream oss;
    oss << "ExecuteDataResponsePacket(cmd=" << toHex(m_clientCmd)
        << ", status=" << toHex(m_status) << ")";
    return oss.str();
}

std::unique_ptr<IPacket> ExecuteDataResponsePacket::clone() const {
    return std::make_unique<ExecuteDataResponsePacket>(*this);
}

RxDataPacket::RxDataPacket(uint32_t imageId, uint32_t dataOffset, uint32_t dataLength)
    : m_imageId(imageId)
    , m_dataOffset(dataOffset)
    , m_dataLength(dataLength) {}

std::string RxDataPacket::toString() const {
    std::ostringstream oss;
    oss << "RxDataPacket(img=" << m_imageId
        << ", offset=" << toHex(m_dataOffset)
        << ", len=" << m_dataLength << ")";
    return oss.str();
}

std::unique_ptr<IPacket> RxDataPacket::clone() const {
    return std::make_unique<RxDataPacket>(*this);
}

RxDataResponsePacket::RxDataResponsePacket(uint32_t imageId, uint32_t status)
    : m_imageId(imageId)
    , m_status(status) {}

std::string RxDataResponsePacket::toString() const {
    std::ostringstream oss;
    oss << "RxDataResponsePacket(img=" << m_imageId
        << ", status=" << toHex(m_status) << ")";
    return oss.str();
}

std::unique_ptr<IPacket> RxDataResponsePacket::clone() const {
    return std::make_unique<RxDataResponsePacket>(*this);
}

std::string ResetStateMachinePacket::toString() const {
    return "ResetStateMachinePacket";
}

std::unique_ptr<IPacket> ResetStateMachinePacket::clone() const {
    return std::make_unique<ResetStateMachinePacket>(*this);
}

} // namespace mbootcore
