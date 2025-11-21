/**
 * JSON format implementation
 */

#include "json.h"
#include <sstream>
#include <cmath>
#include <iomanip>

namespace fconvert {
namespace formats {

bool JSONCodec::is_json(const uint8_t* data, size_t size) {
    if (size < 2) return false;

    // Skip leading whitespace
    size_t i = 0;
    while (i < size && (data[i] == ' ' || data[i] == '\t' ||
                        data[i] == '\n' || data[i] == '\r')) {
        i++;
    }

    // Must start with { or [
    return i < size && (data[i] == '{' || data[i] == '[');
}

void JSONCodec::Parser::skip_whitespace() {
    while (pos < size && (data[pos] == ' ' || data[pos] == '\t' ||
                          data[pos] == '\n' || data[pos] == '\r')) {
        pos++;
    }
}

bool JSONCodec::Parser::match(char c) {
    skip_whitespace();
    if (peek() == c) {
        get();
        return true;
    }
    return false;
}

bool JSONCodec::parse_literal(Parser& p, const char* lit) {
    size_t len = strlen(lit);
    if (p.pos + len > p.size) return false;

    for (size_t i = 0; i < len; i++) {
        if (p.data[p.pos + i] != lit[i]) return false;
    }
    p.pos += len;
    return true;
}

bool JSONCodec::parse_string(Parser& p, std::string& str) {
    if (p.get() != '"') return false;

    str.clear();
    while (p.pos < p.size) {
        char c = p.get();
        if (c == '"') return true;
        if (c == '\\') {
            c = p.get();
            switch (c) {
                case '"': str += '"'; break;
                case '\\': str += '\\'; break;
                case '/': str += '/'; break;
                case 'b': str += '\b'; break;
                case 'f': str += '\f'; break;
                case 'n': str += '\n'; break;
                case 'r': str += '\r'; break;
                case 't': str += '\t'; break;
                case 'u': {
                    // Unicode escape
                    if (p.pos + 4 > p.size) return false;
                    unsigned int codepoint = 0;
                    for (int i = 0; i < 4; i++) {
                        char h = p.get();
                        codepoint <<= 4;
                        if (h >= '0' && h <= '9') codepoint |= h - '0';
                        else if (h >= 'a' && h <= 'f') codepoint |= h - 'a' + 10;
                        else if (h >= 'A' && h <= 'F') codepoint |= h - 'A' + 10;
                        else return false;
                    }
                    // Simple UTF-8 encoding
                    if (codepoint < 0x80) {
                        str += static_cast<char>(codepoint);
                    } else if (codepoint < 0x800) {
                        str += static_cast<char>(0xC0 | (codepoint >> 6));
                        str += static_cast<char>(0x80 | (codepoint & 0x3F));
                    } else {
                        str += static_cast<char>(0xE0 | (codepoint >> 12));
                        str += static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F));
                        str += static_cast<char>(0x80 | (codepoint & 0x3F));
                    }
                    break;
                }
                default: return false;
            }
        } else {
            str += c;
        }
    }
    return false;
}

bool JSONCodec::parse_number(Parser& p, double& num) {
    size_t start = p.pos;

    // Optional minus
    if (p.peek() == '-') p.get();

    // Integer part
    if (p.peek() == '0') {
        p.get();
    } else if (p.peek() >= '1' && p.peek() <= '9') {
        while (p.peek() >= '0' && p.peek() <= '9') p.get();
    } else {
        return false;
    }

    // Fractional part
    if (p.peek() == '.') {
        p.get();
        if (!(p.peek() >= '0' && p.peek() <= '9')) return false;
        while (p.peek() >= '0' && p.peek() <= '9') p.get();
    }

    // Exponent
    if (p.peek() == 'e' || p.peek() == 'E') {
        p.get();
        if (p.peek() == '+' || p.peek() == '-') p.get();
        if (!(p.peek() >= '0' && p.peek() <= '9')) return false;
        while (p.peek() >= '0' && p.peek() <= '9') p.get();
    }

    std::string num_str(p.data + start, p.pos - start);
    num = std::stod(num_str);
    return true;
}

bool JSONCodec::parse_array(Parser& p, JsonArray& arr) {
    if (!p.match('[')) return false;

    arr.clear();
    p.skip_whitespace();

    if (p.peek() == ']') {
        p.get();
        return true;
    }

    while (true) {
        JsonValue value;
        if (!parse_value(p, value)) return false;
        arr.push_back(value);

        p.skip_whitespace();
        if (p.peek() == ']') {
            p.get();
            return true;
        }
        if (!p.match(',')) return false;
    }
}

bool JSONCodec::parse_object(Parser& p, JsonObject& obj) {
    if (!p.match('{')) return false;

    obj.clear();
    p.skip_whitespace();

    if (p.peek() == '}') {
        p.get();
        return true;
    }

    while (true) {
        p.skip_whitespace();
        std::string key;
        if (!parse_string(p, key)) return false;

        if (!p.match(':')) return false;

        JsonValue value;
        if (!parse_value(p, value)) return false;
        obj[key] = value;

        p.skip_whitespace();
        if (p.peek() == '}') {
            p.get();
            return true;
        }
        if (!p.match(',')) return false;
    }
}

bool JSONCodec::parse_value(Parser& p, JsonValue& value) {
    p.skip_whitespace();
    char c = p.peek();

    if (c == '"') {
        std::string str;
        if (!parse_string(p, str)) return false;
        value = str;
        return true;
    }
    if (c == '{') {
        JsonObject obj;
        if (!parse_object(p, obj)) return false;
        value = obj;
        return true;
    }
    if (c == '[') {
        JsonArray arr;
        if (!parse_array(p, arr)) return false;
        value = arr;
        return true;
    }
    if (c == 't') {
        if (!parse_literal(p, "true")) return false;
        value = true;
        return true;
    }
    if (c == 'f') {
        if (!parse_literal(p, "false")) return false;
        value = false;
        return true;
    }
    if (c == 'n') {
        if (!parse_literal(p, "null")) return false;
        value = nullptr;
        return true;
    }
    if (c == '-' || (c >= '0' && c <= '9')) {
        double num;
        if (!parse_number(p, num)) return false;
        value = num;
        return true;
    }

    return false;
}

fconvert_error_t JSONCodec::decode(
    const std::vector<uint8_t>& data,
    JsonValue& root) {

    if (data.empty()) {
        return FCONVERT_ERROR_INVALID_PARAMETER;
    }

    Parser p;
    p.data = reinterpret_cast<const char*>(data.data());
    p.size = data.size();
    p.pos = 0;

    if (!parse_value(p, root)) {
        return FCONVERT_ERROR_INVALID_FORMAT;
    }

    return FCONVERT_OK;
}

void JSONCodec::encode_string(const std::string& str, std::string& out) {
    out += '"';
    for (char c : str) {
        switch (c) {
            case '"': out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\b': out += "\\b"; break;
            case '\f': out += "\\f"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default:
                if (static_cast<unsigned char>(c) < 0x20) {
                    char buf[8];
                    snprintf(buf, sizeof(buf), "\\u%04x", static_cast<unsigned char>(c));
                    out += buf;
                } else {
                    out += c;
                }
        }
    }
    out += '"';
}

void JSONCodec::encode_value(const JsonValue& value, std::string& out, bool pretty, int indent, int depth) {
    std::string ind(depth * indent, ' ');
    std::string ind_inner((depth + 1) * indent, ' ');

    if (value.is_null()) {
        out += "null";
    } else if (value.is_bool()) {
        out += std::get<JsonBool>(value.data) ? "true" : "false";
    } else if (value.is_number()) {
        double num = std::get<JsonNumber>(value.data);
        std::ostringstream oss;
        if (std::floor(num) == num && std::abs(num) < 1e15) {
            oss << static_cast<long long>(num);
        } else {
            oss << std::setprecision(15) << num;
        }
        out += oss.str();
    } else if (value.is_string()) {
        encode_string(std::get<JsonString>(value.data), out);
    } else if (value.is_array()) {
        const JsonArray& arr = std::get<JsonArray>(value.data);
        out += '[';
        if (!arr.empty()) {
            if (pretty) out += '\n';
            for (size_t i = 0; i < arr.size(); i++) {
                if (pretty) out += ind_inner;
                encode_value(arr[i], out, pretty, indent, depth + 1);
                if (i + 1 < arr.size()) out += ',';
                if (pretty) out += '\n';
            }
            if (pretty) out += ind;
        }
        out += ']';
    } else if (value.is_object()) {
        const JsonObject& obj = std::get<JsonObject>(value.data);
        out += '{';
        if (!obj.empty()) {
            if (pretty) out += '\n';
            size_t i = 0;
            for (const auto& [key, val] : obj) {
                if (pretty) out += ind_inner;
                encode_string(key, out);
                out += ':';
                if (pretty) out += ' ';
                encode_value(val, out, pretty, indent, depth + 1);
                if (i + 1 < obj.size()) out += ',';
                if (pretty) out += '\n';
                i++;
            }
            if (pretty) out += ind;
        }
        out += '}';
    }
}

fconvert_error_t JSONCodec::encode(
    const JsonValue& root,
    std::vector<uint8_t>& data,
    bool pretty,
    int indent) {

    std::string out;
    encode_value(root, out, pretty, indent, 0);
    if (pretty) out += '\n';

    data.assign(out.begin(), out.end());
    return FCONVERT_OK;
}

} // namespace formats
} // namespace fconvert
