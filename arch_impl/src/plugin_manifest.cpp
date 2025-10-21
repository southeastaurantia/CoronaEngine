#include "corona/framework/plugin/plugin_manifest.h"

#include <cctype>
#include <fstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace corona::framework::plugin {

namespace {

class json_reader {
   public:
    explicit json_reader(std::string_view input) : text_(input) {}

    plugin_manifest parse_manifest() {
        skip_ws();
        expect('{');
        plugin_manifest manifest;
        bool first = true;
        while (true) {
            skip_ws();
            if (peek() == '}') {
                advance();
                break;
            }
            if (!first) {
                expect(',');
            }
            first = false;
            auto key = parse_string();
            expect(':');
            if (key == "name") {
                manifest.name = parse_string();
            } else if (key == "version") {
                manifest.version = parse_string();
            } else if (key == "dependencies") {
                manifest.dependencies = parse_string_array();
            } else if (key == "systems") {
                manifest.systems = parse_systems();
            } else {
                skip_value();
            }
        }
        return manifest;
    }

   private:
    std::string_view text_;
    std::size_t offset_ = 0;

    char peek() const {
        if (offset_ >= text_.size()) {
            throw std::runtime_error("unexpected end of JSON");
        }
        return text_[offset_];
    }

    void advance() {
        if (offset_ >= text_.size()) {
            throw std::runtime_error("unexpected end of JSON");
        }
        ++offset_;
    }

    void skip_ws() {
        while (offset_ < text_.size()) {
            char c = text_[offset_];
            if (c == ' ' || c == '\n' || c == '\t' || c == '\r') {
                ++offset_;
            } else {
                break;
            }
        }
    }

    void expect(char expected) {
        skip_ws();
        if (peek() != expected) {
            throw std::runtime_error("unexpected character in JSON");
        }
        advance();
    }

    std::string parse_string() {
        skip_ws();
        if (peek() != '"') {
            throw std::runtime_error("expected string");
        }
        advance();
        std::string result;
        while (true) {
            if (offset_ >= text_.size()) {
                throw std::runtime_error("unterminated string");
            }
            char c = text_[offset_++];
            if (c == '"') {
                break;
            }
            if (c == '\\') {
                if (offset_ >= text_.size()) {
                    throw std::runtime_error("invalid escape sequence");
                }
                char esc = text_[offset_++];
                switch (esc) {
                    case '\\':
                        result.push_back('\\');
                        break;
                    case '"':
                        result.push_back('"');
                        break;
                    case 'n':
                        result.push_back('\n');
                        break;
                    case 'r':
                        result.push_back('\r');
                        break;
                    case 't':
                        result.push_back('\t');
                        break;
                    default:
                        throw std::runtime_error("unsupported escape sequence");
                }
            } else {
                result.push_back(c);
            }
        }
        return result;
    }

    std::vector<std::string> parse_string_array() {
        std::vector<std::string> values;
        skip_ws();
        expect('[');
        skip_ws();
        if (peek() == ']') {
            advance();
            return values;
        }
        bool first = true;
        while (true) {
            if (!first) {
                expect(',');
            }
            first = false;
            values.push_back(parse_string());
            skip_ws();
            if (peek() == ']') {
                advance();
                break;
            }
        }
        return values;
    }

    std::vector<plugin_manifest::system_entry> parse_systems() {
        std::vector<plugin_manifest::system_entry> systems;
        skip_ws();
        expect('[');
        skip_ws();
        if (peek() == ']') {
            advance();
            return systems;
        }
        bool first = true;
        while (true) {
            if (!first) {
                expect(',');
            }
            first = false;
            systems.push_back(parse_system_entry());
            skip_ws();
            if (peek() == ']') {
                advance();
                break;
            }
        }
        return systems;
    }

    plugin_manifest::system_entry parse_system_entry() {
        plugin_manifest::system_entry entry;
        skip_ws();
        expect('{');
        bool first = true;
        while (true) {
            skip_ws();
            if (peek() == '}') {
                advance();
                break;
            }
            if (!first) {
                expect(',');
            }
            first = false;
            auto key = parse_string();
            expect(':');
            if (key == "id") {
                entry.id = parse_string();
            } else if (key == "factory") {
                entry.factory = parse_string();
            } else if (key == "dependencies") {
                entry.dependencies = parse_string_array();
            } else if (key == "tags") {
                entry.tags = parse_string_array();
            } else if (key == "tick_ms") {
                entry.tick_interval = std::chrono::milliseconds(parse_integer());
            } else {
                skip_value();
            }
        }
        if (entry.id.empty()) {
            throw std::runtime_error("system entry requires id");
        }
        if (entry.factory.empty()) {
            throw std::runtime_error("system entry requires factory");
        }
        return entry;
    }

    int parse_integer() {
        skip_ws();
        bool negative = false;
        if (peek() == '-') {
            negative = true;
            advance();
        }
        int value = 0;
        bool has_digit = false;
        while (offset_ < text_.size()) {
            char c = text_[offset_];
            if (!std::isdigit(static_cast<unsigned char>(c))) {
                break;
            }
            has_digit = true;
            value = (value * 10) + (c - '0');
            ++offset_;
        }
        if (!has_digit) {
            throw std::runtime_error("expected integer");
        }
        return negative ? -value : value;
    }

    void skip_value() {
        skip_ws();
        char c = peek();
        if (c == '"') {
            parse_string();
            return;
        }
        if (c == '{') {
            advance();
            int depth = 1;
            while (depth > 0) {
                if (offset_ >= text_.size()) {
                    throw std::runtime_error("unterminated object");
                }
                char ch = text_[offset_++];
                if (ch == '{') {
                    ++depth;
                } else if (ch == '}') {
                    --depth;
                }
            }
            return;
        }
        if (c == '[') {
            advance();
            int depth = 1;
            while (depth > 0) {
                if (offset_ >= text_.size()) {
                    throw std::runtime_error("unterminated array");
                }
                char ch = text_[offset_++];
                if (ch == '[') {
                    ++depth;
                } else if (ch == ']') {
                    --depth;
                }
            }
            return;
        }
        // numbers / literals
        while (offset_ < text_.size()) {
            char ch = text_[offset_];
            if (ch == ',' || ch == '}' || ch == ']') {
                break;
            }
            ++offset_;
        }
    }
};

}  // namespace

plugin_manifest parse_manifest(std::string_view json_text) {
    json_reader reader(json_text);
    return reader.parse_manifest();
}

plugin_manifest load_manifest(const std::filesystem::path& path) {
    std::ifstream stream(path);
    if (!stream.is_open()) {
        throw std::runtime_error("failed to open manifest file");
    }
    std::string content((std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());
    return parse_manifest(content);
}

}  // namespace corona::framework::plugin
