#pragma once

#include "mbootcore/domain/Types.hpp"
#include "mbootcore/domain/Error.hpp"
#include "mbootcore/core/protocols/firehose/FirehoseXmlEngine.hpp"

#include <string>
#include <cstdint>
#include <optional>
#include <memory>

namespace mbootcore {

/// --------------------------------------------------------------------------
/// Firehose base command — all typed commands derive from this.
/// Firehose commands do NOT extend IPacket (Firehose is XML, not binary).
/// --------------------------------------------------------------------------
class FirehoseCommand {
public:
    virtual ~FirehoseCommand() = default;

    virtual std::string commandName() const noexcept = 0;
    virtual XmlElement toXml() const = 0;
    virtual bool requiresConfigure() const noexcept { return true; }
    virtual bool hasDataPayload() const noexcept { return false; }
    virtual bool dataFromHost() const noexcept { return true; }

    std::string serialize() const;
};

/// --------------------------------------------------------------------------
/// <configure>
/// --------------------------------------------------------------------------
class ConfigureCommand : public FirehoseCommand {
public:
    std::string memoryName{"ufs"};
    uint32_t zlpAwareHost{1};
    uint32_t skipWrite{0};
    uint32_t maxPayloadSizeToTarget{1048576};
    uint32_t maxPayloadSizeFromTarget{1048576};
    uint32_t mode{0};

    std::string commandName() const noexcept override { return "configure"; }
    XmlElement toXml() const override;
    bool requiresConfigure() const noexcept override { return false; }

    static ConfigureCommand fromXml(const XmlElement& element);
};

/// --------------------------------------------------------------------------
/// <program>
/// --------------------------------------------------------------------------
class ProgramCommand : public FirehoseCommand {
public:
    uint32_t sectorSize{4096};
    uint32_t numSectorSize{0};
    uint32_t partitionId{0};
    uint32_t startSector{0};
    uint32_t physicalPartition{0};
    uint32_t numPartitionSectors{0};
    std::string filename;
    std::string label;

    std::string commandName() const noexcept override { return "program"; }
    XmlElement toXml() const override;
    bool hasDataPayload() const noexcept override { return true; }

    static ProgramCommand fromXml(const XmlElement& element);
};

/// --------------------------------------------------------------------------
/// <read>
/// --------------------------------------------------------------------------
class ReadCommand : public FirehoseCommand {
public:
    uint32_t sectorSize{4096};
    uint32_t numSectorSize{0};
    uint32_t partitionId{0};
    uint32_t startSector{0};
    uint32_t physicalPartition{0};

    std::string commandName() const noexcept override { return "read"; }
    XmlElement toXml() const override;
    bool hasDataPayload() const noexcept override { return true; }
    bool dataFromHost() const noexcept override { return false; }

    static ReadCommand fromXml(const XmlElement& element);
};

/// --------------------------------------------------------------------------
/// <erase>
/// --------------------------------------------------------------------------
class EraseCommand : public FirehoseCommand {
public:
    uint32_t sectorSize{4096};
    uint32_t numSectorSize{0};
    uint32_t partitionId{0};
    uint32_t startSector{0};
    uint32_t physicalPartition{0};
    std::string filename;
    std::string label;

    std::string commandName() const noexcept override { return "erase"; }
    XmlElement toXml() const override;
    static EraseCommand fromXml(const XmlElement& element);
};

/// --------------------------------------------------------------------------
/// <peek>
/// --------------------------------------------------------------------------
class PeekCommand : public FirehoseCommand {
public:
    uint32_t address{0};
    uint32_t size{256};

    std::string commandName() const noexcept override { return "peek"; }
    XmlElement toXml() const override;
    bool requiresConfigure() const noexcept override { return false; }
    static PeekCommand fromXml(const XmlElement& element);
};

/// --------------------------------------------------------------------------
/// <poke>
/// --------------------------------------------------------------------------
class PokeCommand : public FirehoseCommand {
public:
    uint32_t address{0};
    uint32_t size{0};
    ByteBuffer data;

    std::string commandName() const noexcept override { return "poke"; }
    XmlElement toXml() const override;
    bool requiresConfigure() const noexcept override { return false; }
    static PokeCommand fromXml(const XmlElement& element);
};

/// --------------------------------------------------------------------------
/// <patch>
/// --------------------------------------------------------------------------
class PatchCommand : public FirehoseCommand {
public:
    uint32_t byteOffset{0};
    uint32_t size{0};
    uint32_t data{0};
    std::string filename;
    uint32_t sectorSize{4096};
    uint32_t startSector{0};
    uint32_t partitionId{0};
    uint32_t physicalPartition{0};

    std::string commandName() const noexcept override { return "patch"; }
    XmlElement toXml() const override;
    static PatchCommand fromXml(const XmlElement& element);
};

/// --------------------------------------------------------------------------
/// <reset>
/// --------------------------------------------------------------------------
class ResetCommand : public FirehoseCommand {
public:
    std::string value{"reset"};

    std::string commandName() const noexcept override { return "reset"; }
    XmlElement toXml() const override;
    bool requiresConfigure() const noexcept override { return false; }
    static ResetCommand fromXml(const XmlElement& element);
};

/// --------------------------------------------------------------------------
/// <power>
/// --------------------------------------------------------------------------
class PowerCommand : public FirehoseCommand {
public:
    std::string value{"reset"};

    std::string commandName() const noexcept override { return "power"; }
    XmlElement toXml() const override;
    bool requiresConfigure() const noexcept override { return false; }
    static PowerCommand fromXml(const XmlElement& element);
};

/// --------------------------------------------------------------------------
/// <NOP>
/// --------------------------------------------------------------------------
class NopCommand : public FirehoseCommand {
public:
    std::string commandName() const noexcept override { return "NOP"; }
    XmlElement toXml() const override;
    bool requiresConfigure() const noexcept override { return false; }
    static NopCommand fromXml(const XmlElement& element);
};

/// --------------------------------------------------------------------------
/// <getstorageinfo>
/// --------------------------------------------------------------------------
class GetStorageInfoCommand : public FirehoseCommand {
public:
    std::string commandName() const noexcept override { return "getstorageinfo"; }
    XmlElement toXml() const override;
    static GetStorageInfoCommand fromXml(const XmlElement& element);
};

/// --------------------------------------------------------------------------
/// <getsha256digest>
/// --------------------------------------------------------------------------
class GetSha256DigestCommand : public FirehoseCommand {
public:
    uint32_t sectorSize{4096};
    uint32_t numSectorSize{0};
    uint32_t partitionId{0};
    uint32_t startSector{0};
    uint32_t physicalPartition{0};

    std::string commandName() const noexcept override { return "getsha256digest"; }
    XmlElement toXml() const override;
    static GetSha256DigestCommand fromXml(const XmlElement& element);
};

/// --------------------------------------------------------------------------
/// <configurememory>
/// --------------------------------------------------------------------------
class ConfigureMemoryCommand : public FirehoseCommand {
public:
    std::string memoryName{"ufs"};
    std::string config{"all"};

    std::string commandName() const noexcept override { return "configurememory"; }
    XmlElement toXml() const override;
    static ConfigureMemoryCommand fromXml(const XmlElement& element);
};

/// --------------------------------------------------------------------------
/// Firehose Response — typed result from device.
/// --------------------------------------------------------------------------
class FirehoseResponse {
public:
    bool isAck() const noexcept { return m_ack; }
    bool isNak() const noexcept { return m_nak; }
    const std::string& rawXml() const noexcept { return m_rawXml; }
    const std::string& commandName() const noexcept { return m_commandName; }
    const std::string& nakDescription() const noexcept { return m_nakDesc; }

    /// Raw data payload from streaming reads and peek.
    const ByteBuffer& data() const noexcept { return m_data; }
    void setData(ByteBuffer data) { m_data = std::move(data); }

    bool hasRawMode() const noexcept { return m_rawMode; }

    static FirehoseResponse fromXml(const std::string& xml);

private:
    bool m_ack{false};
    bool m_nak{false};
    bool m_rawMode{false};
    std::string m_rawXml;
    std::string m_commandName;
    std::string m_nakDesc;
    ByteBuffer m_data;
};

/// Parse any Firehose XML string into the appropriate typed command.
/// Returns Error if the command is unknown or XML is invalid.
Result<std::unique_ptr<FirehoseCommand>> parseFirehoseXml(const std::string& xml);

} // namespace mbootcore