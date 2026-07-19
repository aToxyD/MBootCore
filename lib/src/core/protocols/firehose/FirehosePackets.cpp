#include "mbootcore/core/protocols/firehose/FirehosePackets.hpp"

#include <sstream>

namespace mbootcore {

namespace {

std::string toHex(uint32_t val) {
    std::ostringstream oss;
    oss << "0x" << std::hex << val;
    return oss.str();
}

} // anonymous namespace

std::string FirehoseCommand::serialize() const {
    auto result = FirehoseXmlEngine::serialize(toXml());
    if (result.isOk()) return result.value();
    return "<error/>";
}

XmlElement ConfigureCommand::toXml() const {
    XmlElement elem;
    elem.name = "configure";
    elem.attributes = {
        {"MemoryName", memoryName},
        {"ZLPAwareHost", std::to_string(zlpAwareHost)},
        {"SkipWrite", std::to_string(skipWrite)},
        {"MaxPayloadSizeToTarget", std::to_string(maxPayloadSizeToTarget)},
        {"MaxPayloadSizeFromTarget", std::to_string(maxPayloadSizeFromTarget)},
        {"Mode", std::to_string(mode)},
    };
    return elem;
}

ConfigureCommand ConfigureCommand::fromXml(const XmlElement& element) {
    ConfigureCommand cmd;
    cmd.memoryName = FirehoseXmlEngine::getAttribute(element, "MemoryName", "ufs");
    cmd.zlpAwareHost = FirehoseXmlEngine::getUintAttribute(element, "ZLPAwareHost", 1);
    cmd.skipWrite = FirehoseXmlEngine::getUintAttribute(element, "SkipWrite", 0);
    cmd.maxPayloadSizeToTarget = FirehoseXmlEngine::getUintAttribute(element, "MaxPayloadSizeToTarget", 1048576);
    cmd.maxPayloadSizeFromTarget = FirehoseXmlEngine::getUintAttribute(element, "MaxPayloadSizeFromTarget", 1048576);
    cmd.mode = FirehoseXmlEngine::getUintAttribute(element, "Mode", 0);
    return cmd;
}

XmlElement ProgramCommand::toXml() const {
    XmlElement elem;
    elem.name = "program";
    elem.attributes = {
        {"SECTOR_SIZE", std::to_string(sectorSize)},
        {"NUM_SECTOR_SIZE", std::to_string(numSectorSize)},
        {"PARTITION_ID", std::to_string(partitionId)},
        {"START_SECTOR", std::to_string(startSector)},
        {"physical_partition", std::to_string(physicalPartition)},
        {"num_partition_sectors", std::to_string(numPartitionSectors)},
    };
    if (!filename.empty()) {
        elem.attributes.push_back({"filename", filename});
    }
    if (!label.empty()) {
        elem.attributes.push_back({"label", label});
    }
    return elem;
}

ProgramCommand ProgramCommand::fromXml(const XmlElement& element) {
    ProgramCommand cmd;
    cmd.sectorSize = FirehoseXmlEngine::getUintAttribute(element, "SECTOR_SIZE", 4096);
    cmd.numSectorSize = FirehoseXmlEngine::getUintAttribute(element, "NUM_SECTOR_SIZE", 0);
    cmd.partitionId = FirehoseXmlEngine::getUintAttribute(element, "PARTITION_ID", 0);
    cmd.startSector = FirehoseXmlEngine::getUintAttribute(element, "START_SECTOR", 0);
    cmd.physicalPartition = FirehoseXmlEngine::getUintAttribute(element, "physical_partition", 0);
    cmd.numPartitionSectors = FirehoseXmlEngine::getUintAttribute(element, "num_partition_sectors", 0);
    cmd.filename = FirehoseXmlEngine::getAttribute(element, "filename", "");
    cmd.label = FirehoseXmlEngine::getAttribute(element, "label", "");
    return cmd;
}

XmlElement ReadCommand::toXml() const {
    XmlElement elem;
    elem.name = "read";
    elem.attributes = {
        {"SECTOR_SIZE", std::to_string(sectorSize)},
        {"NUM_SECTOR_SIZE", std::to_string(numSectorSize)},
        {"PARTITION_ID", std::to_string(partitionId)},
        {"START_SECTOR", std::to_string(startSector)},
        {"physical_partition", std::to_string(physicalPartition)},
    };
    return elem;
}

ReadCommand ReadCommand::fromXml(const XmlElement& element) {
    ReadCommand cmd;
    cmd.sectorSize = FirehoseXmlEngine::getUintAttribute(element, "SECTOR_SIZE", 4096);
    cmd.numSectorSize = FirehoseXmlEngine::getUintAttribute(element, "NUM_SECTOR_SIZE", 0);
    cmd.partitionId = FirehoseXmlEngine::getUintAttribute(element, "PARTITION_ID", 0);
    cmd.startSector = FirehoseXmlEngine::getUintAttribute(element, "START_SECTOR", 0);
    cmd.physicalPartition = FirehoseXmlEngine::getUintAttribute(element, "physical_partition", 0);
    return cmd;
}

XmlElement EraseCommand::toXml() const {
    XmlElement elem;
    elem.name = "erase";
    elem.attributes = {
        {"SECTOR_SIZE", std::to_string(sectorSize)},
        {"NUM_SECTOR_SIZE", std::to_string(numSectorSize)},
        {"PARTITION_ID", std::to_string(partitionId)},
        {"START_SECTOR", std::to_string(startSector)},
        {"physical_partition", std::to_string(physicalPartition)},
    };
    if (!filename.empty()) {
        elem.attributes.push_back({"filename", filename});
    }
    if (!label.empty()) {
        elem.attributes.push_back({"label", label});
    }
    return elem;
}

EraseCommand EraseCommand::fromXml(const XmlElement& element) {
    EraseCommand cmd;
    cmd.sectorSize = FirehoseXmlEngine::getUintAttribute(element, "SECTOR_SIZE", 4096);
    cmd.numSectorSize = FirehoseXmlEngine::getUintAttribute(element, "NUM_SECTOR_SIZE", 0);
    cmd.partitionId = FirehoseXmlEngine::getUintAttribute(element, "PARTITION_ID", 0);
    cmd.startSector = FirehoseXmlEngine::getUintAttribute(element, "START_SECTOR", 0);
    cmd.physicalPartition = FirehoseXmlEngine::getUintAttribute(element, "physical_partition", 0);
    cmd.filename = FirehoseXmlEngine::getAttribute(element, "filename", "");
    cmd.label = FirehoseXmlEngine::getAttribute(element, "label", "");
    return cmd;
}

XmlElement PeekCommand::toXml() const {
    XmlElement elem;
    elem.name = "peek";
    elem.attributes = {
        {"address", toHex(address)},
        {"size", std::to_string(size)},
    };
    return elem;
}

PeekCommand PeekCommand::fromXml(const XmlElement& element) {
    PeekCommand cmd;
    cmd.address = FirehoseXmlEngine::getUintAttribute(element, "address", 0);
    cmd.size = FirehoseXmlEngine::getUintAttribute(element, "size", 256);
    return cmd;
}

namespace {
std::string toHex(const uint8_t* buf, size_t len) {
    static const char hex[] = "0123456789ABCDEF";
    std::string result;
    result.reserve(len * 2);
    for (size_t i = 0; i < len; ++i) {
        result += hex[(buf[i] >> 4) & 0xF];
        result += hex[buf[i] & 0xF];
    }
    return result;
}

ByteBuffer fromHex(const std::string& hex) {
    if (hex.size() % 2 != 0) {
        return {};
    }

    auto toNibble = [](char c, uint8_t& value) -> bool {
        if (c >= '0' && c <= '9') {
            value = static_cast<uint8_t>(c - '0');
            return true;
        }
        if (c >= 'A' && c <= 'F') {
            value = static_cast<uint8_t>(c - 'A' + 10);
            return true;
        }
        if (c >= 'a' && c <= 'f') {
            value = static_cast<uint8_t>(c - 'a' + 10);
            return true;
        }
        return false;
    };

    ByteBuffer result;
    result.reserve(hex.size() / 2);

    for (size_t i = 0; i < hex.size(); i += 2) {
        uint8_t hi = 0;
        uint8_t lo = 0;

        if (!toNibble(hex[i], hi) ||
            !toNibble(hex[i + 1], lo)) {
            return {};
        }

        result.push_back(static_cast<uint8_t>((hi << 4) | lo));
    }

    return result;
}
} // anonymous namespace

XmlElement PokeCommand::toXml() const {
    XmlElement elem;
    elem.name = "poke";
    elem.attributes = {
        {"address", toHex(address)},
        {"size", std::to_string(size)},
    };
    if (!data.empty()) {
        elem.attributes.push_back({"data", toHex(data.data(), data.size())});
    }
    return elem;
}

PokeCommand PokeCommand::fromXml(const XmlElement& element) {
    PokeCommand cmd;
    cmd.address = FirehoseXmlEngine::getUintAttribute(element, "address", 0);
    cmd.size = FirehoseXmlEngine::getUintAttribute(element, "size", 0);
    auto dataHex = FirehoseXmlEngine::getAttribute(element, "data", "");
    if (!dataHex.empty()) {
        cmd.data = fromHex(dataHex);
    }
    return cmd;
}

XmlElement PatchCommand::toXml() const {
    XmlElement elem;
    elem.name = "patch";
    elem.attributes = {
        {"byte_offset", toHex(byteOffset)},
        {"size", std::to_string(size)},
        {"data", toHex(data)},
        {"filename", filename},
        {"SECTOR_SIZE", std::to_string(sectorSize)},
        {"START_SECTOR", std::to_string(startSector)},
        {"PARTITION_ID", std::to_string(partitionId)},
        {"physical_partition", std::to_string(physicalPartition)},
    };
    return elem;
}

PatchCommand PatchCommand::fromXml(const XmlElement& element) {
    PatchCommand cmd;
    cmd.byteOffset = FirehoseXmlEngine::getUintAttribute(element, "byte_offset", 0);
    cmd.size = FirehoseXmlEngine::getUintAttribute(element, "size", 0);
    cmd.data = FirehoseXmlEngine::getUintAttribute(element, "data", 0);
    cmd.filename = FirehoseXmlEngine::getAttribute(element, "filename", "");
    cmd.sectorSize = FirehoseXmlEngine::getUintAttribute(element, "SECTOR_SIZE", 4096);
    cmd.startSector = FirehoseXmlEngine::getUintAttribute(element, "START_SECTOR", 0);
    cmd.partitionId = FirehoseXmlEngine::getUintAttribute(element, "PARTITION_ID", 0);
    cmd.physicalPartition = FirehoseXmlEngine::getUintAttribute(element, "physical_partition", 0);
    return cmd;
}

XmlElement ResetCommand::toXml() const {
    XmlElement elem;
    elem.name = "reset";
    elem.attributes = {{"value", value}};
    return elem;
}

ResetCommand ResetCommand::fromXml(const XmlElement& element) {
    ResetCommand cmd;
    cmd.value = FirehoseXmlEngine::getAttribute(element, "value", "reset");
    return cmd;
}

XmlElement PowerCommand::toXml() const {
    XmlElement elem;
    elem.name = "power";
    elem.attributes = {{"value", value}};
    return elem;
}

PowerCommand PowerCommand::fromXml(const XmlElement& element) {
    PowerCommand cmd;
    cmd.value = FirehoseXmlEngine::getAttribute(element, "value", "reset");
    return cmd;
}

XmlElement NopCommand::toXml() const {
    XmlElement elem;
    elem.name = "NOP";
    return elem;
}

NopCommand NopCommand::fromXml(const XmlElement&) {
    return NopCommand{};
}

XmlElement GetStorageInfoCommand::toXml() const {
    XmlElement elem;
    elem.name = "getstorageinfo";
    return elem;
}

GetStorageInfoCommand GetStorageInfoCommand::fromXml(const XmlElement&) {
    return GetStorageInfoCommand{};
}

XmlElement GetSha256DigestCommand::toXml() const {
    XmlElement elem;
    elem.name = "getsha256digest";
    elem.attributes = {
        {"SECTOR_SIZE", std::to_string(sectorSize)},
        {"NUM_SECTOR_SIZE", std::to_string(numSectorSize)},
        {"PARTITION_ID", std::to_string(partitionId)},
        {"START_SECTOR", std::to_string(startSector)},
        {"physical_partition", std::to_string(physicalPartition)},
    };
    return elem;
}

GetSha256DigestCommand GetSha256DigestCommand::fromXml(const XmlElement& element) {
    GetSha256DigestCommand cmd;
    cmd.sectorSize = FirehoseXmlEngine::getUintAttribute(element, "SECTOR_SIZE", 4096);
    cmd.numSectorSize = FirehoseXmlEngine::getUintAttribute(element, "NUM_SECTOR_SIZE", 0);
    cmd.partitionId = FirehoseXmlEngine::getUintAttribute(element, "PARTITION_ID", 0);
    cmd.startSector = FirehoseXmlEngine::getUintAttribute(element, "START_SECTOR", 0);
    cmd.physicalPartition = FirehoseXmlEngine::getUintAttribute(element, "physical_partition", 0);
    return cmd;
}

XmlElement ConfigureMemoryCommand::toXml() const {
    XmlElement elem;
    elem.name = "configurememory";
    elem.attributes = {
        {"MemoryName", memoryName},
        {"Config", config},
    };
    return elem;
}

ConfigureMemoryCommand ConfigureMemoryCommand::fromXml(const XmlElement& element) {
    ConfigureMemoryCommand cmd;
    cmd.memoryName = FirehoseXmlEngine::getAttribute(element, "MemoryName", "ufs");
    cmd.config = FirehoseXmlEngine::getAttribute(element, "Config", "all");
    return cmd;
}

FirehoseResponse FirehoseResponse::fromXml(const std::string& xml) {
    FirehoseResponse resp;
    resp.m_rawXml = xml;

    auto parsed = FirehoseXmlEngine::parse(xml);
    if (!parsed.isOk()) return resp;

    auto& element = parsed.value();
    resp.m_commandName = element.name;
    resp.m_ack = FirehoseXmlEngine::isAck(element);
    resp.m_nak = FirehoseXmlEngine::isNak(element);
    resp.m_nakDesc = FirehoseXmlEngine::nakDescription(element);
    resp.m_rawMode = (FirehoseXmlEngine::getAttribute(element, "raw_mode", "") == "true");

    return resp;
}

Result<std::unique_ptr<FirehoseCommand>> parseFirehoseXml(const std::string& xml) {
    auto parsed = FirehoseXmlEngine::parse(xml);
    if (!parsed.isOk()) {
        return parsed.error();
    }

    auto& element = parsed.value();
    const auto& name = element.name;

    std::unique_ptr<FirehoseCommand> cmd;

    if (name == "configure") {
        cmd = std::make_unique<ConfigureCommand>(ConfigureCommand::fromXml(element));
    } else if (name == "program") {
        cmd = std::make_unique<ProgramCommand>(ProgramCommand::fromXml(element));
    } else if (name == "read") {
        cmd = std::make_unique<ReadCommand>(ReadCommand::fromXml(element));
    } else if (name == "erase") {
        cmd = std::make_unique<EraseCommand>(EraseCommand::fromXml(element));
    } else if (name == "peek") {
        cmd = std::make_unique<PeekCommand>(PeekCommand::fromXml(element));
    } else if (name == "poke") {
        cmd = std::make_unique<PokeCommand>(PokeCommand::fromXml(element));
    } else if (name == "patch") {
        cmd = std::make_unique<PatchCommand>(PatchCommand::fromXml(element));
    } else if (name == "reset") {
        cmd = std::make_unique<ResetCommand>(ResetCommand::fromXml(element));
    } else if (name == "power") {
        cmd = std::make_unique<PowerCommand>(PowerCommand::fromXml(element));
    } else if (name == "NOP") {
        cmd = std::make_unique<NopCommand>(NopCommand::fromXml(element));
    } else if (name == "getstorageinfo") {
        cmd = std::make_unique<GetStorageInfoCommand>(GetStorageInfoCommand::fromXml(element));
    } else if (name == "getsha256digest") {
        cmd = std::make_unique<GetSha256DigestCommand>(GetSha256DigestCommand::fromXml(element));
    } else if (name == "configurememory") {
        cmd = std::make_unique<ConfigureMemoryCommand>(ConfigureMemoryCommand::fromXml(element));
    } else {
        return ErrorCode::NotSupported;
    }

    return cmd;
}

} // namespace mbootcore