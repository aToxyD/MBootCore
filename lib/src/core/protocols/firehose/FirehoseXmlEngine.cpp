#include "mbootcore/core/protocols/firehose/FirehoseXmlEngine.hpp"

#include "SafeParser.hpp"

#include <cstring>
#include <cctype>
#include <sstream>

namespace mbootcore {

namespace {

bool isWhitespace(char c) noexcept {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

std::string_view trim(std::string_view s) noexcept {
    while (!s.empty() && isWhitespace(s.front())) s.remove_prefix(1);
    while (!s.empty() && isWhitespace(s.back())) s.remove_suffix(1);
    return s;
}

enum class TokenType { TagOpen, TagClose, SelfClose, AttrName, AttrValue, Text, Eof };

constexpr size_t kMaxXmlDepth = 64;

struct Token {
    TokenType type;
    std::string value;
};

class XmlTokenizer {
public:
    explicit XmlTokenizer(std::string_view input) : m_input(input) {}

    Token next() {
        skipWhitespace();
        if (m_pos >= m_input.size()) return {TokenType::Eof, ""};

        if (m_input[m_pos] == '<') {
            if (m_pos + 1 < m_input.size() && m_input[m_pos + 1] == '/') {
                // Closing tag
                m_pos += 2;
                auto end = m_input.find('>', m_pos);
                if (end == std::string_view::npos) return {TokenType::Eof, ""};
                auto name = std::string(m_input.substr(m_pos, end - m_pos));
                m_pos = end + 1;
                return {TokenType::TagClose, std::move(name)};
            }
            // Opening or self-closing tag
            m_pos++;
            auto end = m_input.find('>', m_pos);
            if (end == std::string_view::npos) return {TokenType::Eof, ""};
            if (end > 0 && m_input[end - 1] == '/') {
                auto content = std::string(m_input.substr(m_pos, end - m_pos - 1));
                m_pos = end + 1;
                return {TokenType::SelfClose, std::move(content)};
            }
            auto content = std::string(m_input.substr(m_pos, end - m_pos));
            m_pos = end + 1;
            return {TokenType::TagOpen, std::move(content)};
        }

        // Text content
        auto end = m_input.find('<', m_pos);
        auto text = std::string(m_input.substr(m_pos, end - m_pos));
        m_pos = (end == std::string_view::npos) ? m_input.size() : end;
        return {TokenType::Text, std::move(text)};
    }

private:
    void skipWhitespace() {
        while (m_pos < m_input.size() && isWhitespace(m_input[m_pos])) m_pos++;
    }

    std::string_view m_input;
    size_t m_pos{0};
};

struct ParsedTag {
    std::string name;
    std::vector<XmlAttribute> attrs;
    bool selfClose{false};
};

ParsedTag parseTagContent(const std::string& content) {
    ParsedTag tag;
    size_t pos = 0;

    // Read tag name
    while (pos < content.size() && !isWhitespace(content[pos])) pos++;
    tag.name = content.substr(0, pos);

    // Read attributes
    while (pos < content.size()) {
        while (pos < content.size() && isWhitespace(content[pos])) pos++;
        if (pos >= content.size()) break;

        // Attribute name
        size_t nameStart = pos;
        while (pos < content.size() && content[pos] != '=' && !isWhitespace(content[pos])) pos++;
        std::string attrName = content.substr(nameStart, pos - nameStart);

        // Skip '='
        if (pos < content.size() && content[pos] == '=') pos++;

        // Skip whitespace
        while (pos < content.size() && isWhitespace(content[pos])) pos++;

        // Attribute value (quoted)
        if (pos < content.size() && content[pos] == '"') {
            pos++; // skip opening quote
            size_t valStart = pos;
            while (pos < content.size() && content[pos] != '"') pos++;
            std::string attrValue = content.substr(valStart, pos - valStart);
            if (pos < content.size()) pos++; // skip closing quote
            tag.attrs.push_back({std::move(attrName), std::move(attrValue)});
        }
    }

    return tag;
}

// depth == current nesting level (0-based; 0 = root element)
Result<XmlElement> parseXmlRecursive(XmlTokenizer& tokenizer, size_t depth) {
    if (depth >= kMaxXmlDepth) {
        return ErrorCode::InvalidPacket;
    }

    auto token = tokenizer.next();
    if (token.type == TokenType::Eof) {
        return ErrorCode::InvalidPacket;
    }
    if (token.type != TokenType::TagOpen && token.type != TokenType::SelfClose) {
        return ErrorCode::InvalidPacket;
    }

    auto tag = parseTagContent(token.value);
    XmlElement element;
    element.name = std::move(tag.name);
    element.attributes = std::move(tag.attrs);

    if (token.type == TokenType::SelfClose) {
        return element;
    }

    // Parse children and text content
    while (true) {
        auto nextToken = tokenizer.next();
        if (nextToken.type == TokenType::Eof) break;
        if (nextToken.type == TokenType::TagClose) {
            if (nextToken.value == element.name) break;
            // Mismatched close tag — return what we have
            break;
        }
        if (nextToken.type == TokenType::Text) {
            auto trimmed = trim(nextToken.value);
            if (!trimmed.empty()) {
                element.content = std::string(trimmed);
            }
            continue;
        }
        if (nextToken.type == TokenType::TagOpen || nextToken.type == TokenType::SelfClose) {
            // Re-parse this as a child element
            // We need to put the token back. Since we can't unread, we reconstruct.
            // Instead, we call parseXmlRecursive with the tag content we already have.
            auto childTag = parseTagContent(nextToken.value);
            XmlElement child;
            child.name = std::move(childTag.name);
            child.attributes = std::move(childTag.attrs);
            if (nextToken.type == TokenType::SelfClose) {
                element.children.push_back(std::move(child));
                continue;
            }
            // Parse children of child
            auto childResult = parseXmlRecursive(tokenizer, depth + 1);
            if (!childResult.isOk()) {
                return childResult.error();
            }
            auto childElement = std::move(childResult.value());
            child.children = std::move(childElement.children);
            child.content = std::move(childElement.content);
            element.children.push_back(std::move(child));
        }
    }

    return element;
}

} // anonymous namespace

Result<XmlElement> FirehoseXmlEngine::parse(const std::string& xml) noexcept {
    if (xml.empty()) {
        return ErrorCode::InvalidPacket;
    }
    XmlTokenizer tokenizer(xml);
    // depth == 0: root nesting level
    return parseXmlRecursive(tokenizer, 0);
}

std::string FirehoseXmlEngine::escapeAttribute(const std::string& value) noexcept {
    std::string result;
    result.reserve(value.size());
    for (char c : value) {
        switch (c) {
            case '&':  result += "&amp;"; break;
            case '"':  result += "&quot;"; break;
            case '<':  result += "&lt;"; break;
            case '>':  result += "&gt;"; break;
            case '\'': result += "&apos;"; break;
            default:   result += c; break;
        }
    }
    return result;
}

std::string FirehoseXmlEngine::escapeContent(const std::string& content) noexcept {
    std::string result;
    result.reserve(content.size());
    for (char c : content) {
        switch (c) {
            case '&':  result += "&amp;"; break;
            case '<':  result += "&lt;"; break;
            case '>':  result += "&gt;"; break;
            default:   result += c; break;
        }
    }
    return result;
}

Result<std::string> FirehoseXmlEngine::serialize(const XmlElement& element) noexcept {
    std::ostringstream oss;
    oss << '<' << element.name;
    for (const auto& attr : element.attributes) {
        oss << ' ' << attr.name << "=\"" << escapeAttribute(attr.value) << '"';
    }
    if (element.children.empty() && element.content.empty()) {
        oss << "/>";
        return oss.str();
    }
    oss << '>';
    if (!element.content.empty()) {
        oss << escapeContent(element.content);
    }
    for (const auto& child : element.children) {
        auto childResult = serialize(child);
        if (childResult.isOk()) {
            oss << childResult.value();
        }
    }
    oss << "</" << element.name << '>';
    return oss.str();
}

bool FirehoseXmlEngine::hasAttribute(const XmlElement& element, const std::string& name) noexcept {
    for (const auto& attr : element.attributes) {
        if (attr.name == name) return true;
    }
    return false;
}

std::string FirehoseXmlEngine::getAttribute(const XmlElement& element, const std::string& name,
                                              const std::string& defaultValue) noexcept {
    for (const auto& attr : element.attributes) {
        if (attr.name == name) return attr.value;
    }
    return defaultValue;
}

uint32_t FirehoseXmlEngine::getUintAttribute(const XmlElement& element, const std::string& name,
                                               uint32_t defaultValue) noexcept {
    auto val = getAttribute(element, name, "");
    if (val.empty()) return defaultValue;
    auto result = fromCharsUint32(val);
    if (!result.ok) return defaultValue;
    return result.value;
}

bool FirehoseXmlEngine::isAck(const XmlElement& element) noexcept {
    return getAttribute(element, "value") == "ACK";
}

bool FirehoseXmlEngine::isNak(const XmlElement& element) noexcept {
    return getAttribute(element, "value") == "NAK";
}

std::string FirehoseXmlEngine::nakDescription(const XmlElement& element) noexcept {
    return getAttribute(element, "description", "");
}

} // namespace mbootcore