#pragma once

#include "mbootcore/domain/Error.hpp"
#include "mbootcore/domain/Types.hpp"

#include <string>
#include <vector>
#include <unordered_map>
#include <cstdint>

namespace mbootcore {

struct XmlAttribute {
    std::string name;
    std::string value;
};

struct XmlElement {
    std::string name;
    std::vector<XmlAttribute> attributes;
    std::vector<XmlElement> children;
    std::string content;
};

class FirehoseXmlEngine {
public:
    static Result<XmlElement> parse(const std::string& xml) noexcept;
    static Result<std::string> serialize(const XmlElement& element) noexcept;

    static bool hasAttribute(const XmlElement& element, const std::string& name) noexcept;
    static std::string getAttribute(const XmlElement& element, const std::string& name,
                                     const std::string& defaultValue = "") noexcept;
    static uint32_t getUintAttribute(const XmlElement& element, const std::string& name,
                                      uint32_t defaultValue = 0) noexcept;
    static bool isAck(const XmlElement& element) noexcept;
    static bool isNak(const XmlElement& element) noexcept;
    static std::string nakDescription(const XmlElement& element) noexcept;

    static std::string escapeAttribute(const std::string& value) noexcept;
    static std::string escapeContent(const std::string& content) noexcept;
};

} // namespace mbootcore