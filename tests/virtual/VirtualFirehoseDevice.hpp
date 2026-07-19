#pragma once

#include "mbootcore/core/protocols/firehose/FirehoseXmlEngine.hpp"
#include "mbootcore/core/protocols/firehose/FirehosePackets.hpp"
#include "MockTransport.hpp"

#include <string>
#include <deque>
#include <unordered_map>
#include <vector>
#include <array>
#include <functional>
#include <random>
#include <algorithm>

namespace mbootcore {

/// Mode for fault injection
enum class FaultMode {
    None,
    NackAll,
    CrcMismatch,
    ShortResponse,
    DisconnectAfter,
    InvalidXml,
    TimeoutOnChunk,
    PartialWrite,
    RandomNak
};

/// Virtual Firehose Device — full simulation with storage and fault injection.
class VirtualFirehoseDevice {
public:
    explicit VirtualFirehoseDevice(MockTransport& transport)
        : m_transport(transport) {}

    // -----------------------------------------------------------------------
    // Response queuing (original API)
    // -----------------------------------------------------------------------
    void queueResponse(const std::string& xml) {
        ByteBuffer buf(xml.begin(), xml.end());
        buf.push_back(0);
        m_transport.setReadData(buf);
    }

    void queueAck(const std::string& command) {
        queueResponse("<" + command + " value=\"ACK\"/>");
    }

    void queueNak(const std::string& command, const std::string& desc = "Error") {
        queueResponse("<" + command + " value=\"NAK\" description=\"" + desc + "\"/>");
    }

    void queueConfigureAck(const ConfigureCommand& cmd) {
        auto ack = cmd.toXml();
        ack.attributes.push_back({"value", "ACK"});
        auto result = FirehoseXmlEngine::serialize(ack);
        if (result.isOk()) queueResponse(result.value());
    }

    void queueReadAck() {
        queueResponse("<read value=\"ACK\" raw_mode=\"true\"/>");
    }

    const ByteBuffer& allWrites() const { return m_allWrites; }

    void captureWrites() { m_captureWrites = true; }

    void checkAndCapture(const MockTransport::WriteRecord& rec) {
        if (m_captureWrites) {
            m_allWrites.insert(m_allWrites.end(), rec.data.begin(), rec.data.end());
        }
    }

    // -----------------------------------------------------------------------
    // Virtual storage (sector-based)
    // -----------------------------------------------------------------------
    void configureStorage(uint64_t numSectors, uint32_t sectorSize = 4096) {
        m_numSectors = numSectors;
        m_sectorSize = sectorSize;
        m_storage.assign(numSectors, ByteBuffer(sectorSize, 0xFF));
    }

    void setSectorData(uint64_t sector, const ByteBuffer& data) {
        if (sector < m_storage.size()) {
            size_t copySize = std::min(data.size(), m_storage[sector].size());
            std::memcpy(m_storage[sector].data(), data.data(), copySize);
        }
    }

    ByteBuffer readStorage(uint64_t startSector, uint64_t numSectors) const {
        ByteBuffer result;
        for (uint64_t i = 0; i < numSectors && (startSector + i) < m_storage.size(); ++i) {
            const auto& sector = m_storage[startSector + i];
            result.insert(result.end(), sector.begin(), sector.end());
        }
        return result;
    }

    void eraseStorage(uint64_t startSector, uint64_t numSectors) {
        for (uint64_t i = 0; i < numSectors && (startSector + i) < m_storage.size(); ++i) {
            std::fill(m_storage[startSector + i].begin(),
                      m_storage[startSector + i].end(), 0xFF);
        }
    }

    // -----------------------------------------------------------------------
    // Virtual memory (for peek/poke)
    // -----------------------------------------------------------------------
    ByteBuffer readMemory(uint64_t address, uint32_t size) const {
        ByteBuffer result;
        if (address < m_memory.size()) {
            size_t copySize = std::min(static_cast<size_t>(size),
                                       m_memory.size() - static_cast<size_t>(address));
            result.assign(m_memory.begin() + static_cast<ptrdiff_t>(address),
                          m_memory.begin() + static_cast<ptrdiff_t>(address + copySize));
        }
        return result;
    }

    void writeMemory(uint64_t address, const ByteBuffer& data) {
        if (address + data.size() <= m_memory.size()) {
            std::memcpy(m_memory.data() + address, data.data(), data.size());
        }
    }

    // -----------------------------------------------------------------------
    // Fault injection
    // -----------------------------------------------------------------------
    void setFaultMode(FaultMode mode) { m_faultMode = mode; }
    FaultMode faultMode() const { return m_faultMode; }

    // -----------------------------------------------------------------------
    // Auto-handler (called by test harness for each write from host)
    // -----------------------------------------------------------------------
    void handleWrite(const MockTransport::WriteRecord& rec) {
        checkAndCapture(rec);

        if (m_faultAfterOps > 0 && ++m_opCount >= m_faultAfterOps) {
            if (m_faultMode == FaultMode::DisconnectAfter) {
                m_transport.setReadResult(
                    Result<size_t>::Error(ErrorCode::TransportDisconnected));
            }
        }

        std::string xml(rec.data.begin(), rec.data.end());
        auto nullPos = xml.find('\0');
        if (nullPos != std::string::npos) {
            xml = xml.substr(0, nullPos);
        }

        if (m_faultMode == FaultMode::InvalidXml) {
            queueResponse("<<<invalid>>>");
            m_faultMode = FaultMode::None;
            return;
        }

        auto parsed = FirehoseXmlEngine::parse(xml);
        if (!parsed.isOk()) return;

        auto& element = parsed.value();
        const auto& cmdName = element.name;

        if (m_faultMode == FaultMode::RandomNak && (rand() % 3 == 0)) {
            queueNak(cmdName, "Random fault");
            // Do NOT handle the command — NAK was sent
            return;
        }

        if (cmdName == "configure") {
            auto cfg = ConfigureCommand::fromXml(element);
            queueConfigureAck(cfg);
        } else if (cmdName == "program") {
            handleProgramWrite();
        } else if (cmdName == "read") {
            auto readCmd = ReadCommand::fromXml(element);
            handleReadCommand(readCmd);
        } else if (cmdName == "erase") {
            auto eraseCmd = EraseCommand::fromXml(element);
            handleEraseCommand(eraseCmd);
        } else if (cmdName == "peek") {
            auto peekCmd = PeekCommand::fromXml(element);
            handlePeekCommand(peekCmd);
        } else if (cmdName == "poke") {
            auto pokeCmd = PokeCommand::fromXml(element);
            handlePokeCommand(pokeCmd);
        } else if (cmdName == "reset") {
            queueAck("reset");
        } else if (cmdName == "power") {
            queueAck("power");
        } else if (cmdName == "NOP") {
            queueAck("NOP");
        } else if (cmdName == "getstorageinfo") {
            handleStorageInfoCommand();
        } else if (cmdName == "getsha256digest") {
            handleSha256Command();
        } else if (cmdName == "configurememory") {
            queueAck("configurememory");
        } else if (cmdName == "patch") {
            handlePatchCommand(element);
        } else {
            queueNak("unknown", "Unknown command: " + cmdName);
        }
    }

private:
    void handleProgramWrite() {
        if (m_faultMode == FaultMode::NackAll) {
            queueNak("program", "Fault injection NAK");
            return;
        }
        if (m_faultMode == FaultMode::PartialWrite) {
            queueAck("program");
            queueNak("program", "Partial write - incomplete");
            m_faultMode = FaultMode::None;
            return;
        }
        queueAck("program");
    }

    void handleReadCommand(const ReadCommand& cmd) {
        if (m_faultMode == FaultMode::NackAll) {
            queueNak("read", "Fault injection NAK");
            return;
        }

        queueReadAck();

        uint64_t startSector = cmd.startSector;
        uint64_t numSectors = cmd.numSectorSize;
        auto data = readStorage(startSector, numSectors);

        if (m_faultMode == FaultMode::ShortResponse && data.size() > 64) {
            data.resize(64);
            m_faultMode = FaultMode::None;
        } else if (m_faultMode == FaultMode::CrcMismatch && !data.empty()) {
            data[0] ^= 0xFF;
            m_faultMode = FaultMode::None;
        }

        m_transport.setReadData(data);
    }

    void handleEraseCommand(const EraseCommand& cmd) {
        if (m_faultMode == FaultMode::NackAll) {
            queueNak("erase", "Fault injection NAK");
            return;
        }

        if (std::find(m_badSectors.begin(), m_badSectors.end(),
                       cmd.startSector) != m_badSectors.end()) {
            queueNak("erase", "Bad block at sector " + std::to_string(cmd.startSector));
            return;
        }

        if (cmd.startSector < m_storage.size()) {
            eraseStorage(cmd.startSector, cmd.numSectorSize);
        }
        queueAck("erase");
    }

    void handlePeekCommand(const PeekCommand& cmd) {
        auto data = readMemory(cmd.address, cmd.size);
        m_transport.setReadData(data);
        queueAck("peek");
    }

    void handlePokeCommand(const PokeCommand& cmd) {
        writeMemory(cmd.address, cmd.data);
        queueAck("poke");
    }

    void handlePatchCommand(const XmlElement& element) {
        if (element.name != "patch") {
            queueNak("patch", "Invalid element");
            return;
        }
        queueAck("patch");
    }

    void handleStorageInfoCommand() {
        std::string xml = R"(<getstorageinfo value="ACK">
  <storage_info>
    <version>1</version>
    <memb_type>UFS</memb_type>
    <num_entires>1</num_entires>
    <lun_info>
      <lun>0</lun>
      <lun_type>UFS</lun_type>
      <size>)" + std::to_string(m_numSectors) + R"(</size>
      <attributes>1</attributes>
      <bdev_name>sda</bdev_name>
    </lun_info>
  </storage_info>
</getstorageinfo>)";
        queueResponse(xml);
    }

    void handleSha256Command() {
        queueAck("getsha256digest");
        ByteBuffer hash(32, 0xAA);
        m_transport.setReadData(hash);
    }

    static uint32_t crc32(const uint8_t* data, size_t len) {
        uint32_t crc = 0xFFFFFFFF;
        for (size_t i = 0; i < len; ++i) {
            crc ^= data[i];
            for (int j = 0; j < 8; ++j) {
                if (crc & 1) {
                    crc = (crc >> 1) ^ 0xEDB88320;
                } else {
                    crc >>= 1;
                }
            }
        }
        return crc ^ 0xFFFFFFFF;
    }

    MockTransport& m_transport;
    ByteBuffer m_allWrites;
    bool m_captureWrites{false};

    // Storage
    std::vector<ByteBuffer> m_storage;
    uint64_t m_numSectors{0};
    uint32_t m_sectorSize{4096};
    ByteBuffer m_memory;
    std::vector<uint64_t> m_badSectors;

    // Fault injection
    FaultMode m_faultMode{FaultMode::None};
    size_t m_faultAfterOps{0};
    size_t m_opCount{0};
};

inline void setupFirehoseSuccessResponses(VirtualFirehoseDevice& dev) {
    ConfigureCommand cfg;
    dev.queueConfigureAck(cfg);
}

} // namespace mbootcore
