/**
 * JSON (JavaScript Object Notation) format
 */

#ifndef JSON_H
#define JSON_H

#include "../../../include/fconvert.h"
#include <vector>
#include <string>
#include <map>
#include <variant>
#include <memory>

namespace fconvert {
namespace formats {

// Forward declaration
struct JsonValue;

// JSON value types
using JsonNull = std::nullptr_t;
using JsonBool = bool;
using JsonNumber = double;
using JsonString = std::string;
using JsonArray = std::vector<JsonValue>;
using JsonObject = std::map<std::string, JsonValue>;

// JSON value variant
struct JsonValue {
    std::variant<JsonNull, JsonBool, JsonNumber, JsonString, JsonArray, JsonObject> data;

    JsonValue() : data(nullptr) {}
    JsonValue(std::nullptr_t) : data(nullptr) {}
    JsonValue(bool b) : data(b) {}
    JsonValue(double d) : data(d) {}
    JsonValue(int i) : data(static_cast<double>(i)) {}
    JsonValue(const std::string& s) : data(s) {}
    JsonValue(const char* s) : data(std::string(s)) {}
    JsonValue(const JsonArray& a) : data(a) {}
    JsonValue(const JsonObject& o) : data(o) {}

    bool is_null() const { return std::holds_alternative<JsonNull>(data); }
    bool is_bool() const { return std::holds_alternative<JsonBool>(data); }
    bool is_number() const { return std::holds_alternative<JsonNumber>(data); }
    bool is_string() const { return std::holds_alternative<JsonString>(data); }
    bool is_array() const { return std::holds_alternative<JsonArray>(data); }
    bool is_object() const { return std::holds_alternative<JsonObject>(data); }
};

/**
 * JSON codec
 */
class JSONCodec {
public:
    /**
     * Decode JSON data
     */
    static fconvert_error_t decode(
        const std::vector<uint8_t>& data,
        JsonValue& root);

    /**
     * Encode JSON data
     */
    static fconvert_error_t encode(
        const JsonValue& root,
        std::vector<uint8_t>& data,
        bool pretty = true,
        int indent = 2);

    /**
     * Check if data is JSON
     */
    static bool is_json(const uint8_t* data, size_t size);

private:
    // Parser state
    struct Parser {
        const char* data;
        size_t size;
        size_t pos;

        char peek() const { return pos < size ? data[pos] : '\0'; }
        char get() { return pos < size ? data[pos++] : '\0'; }
        void skip_whitespace();
        bool match(char c);
    };

    // Parsing functions
    static bool parse_value(Parser& p, JsonValue& value);
    static bool parse_object(Parser& p, JsonObject& obj);
    static bool parse_array(Parser& p, JsonArray& arr);
    static bool parse_string(Parser& p, std::string& str);
    static bool parse_number(Parser& p, double& num);
    static bool parse_literal(Parser& p, const char* lit);

    // Encoding functions
    static void encode_value(const JsonValue& value, std::string& out, bool pretty, int indent, int depth);
    static void encode_string(const std::string& str, std::string& out);
};

} // namespace formats
} // namespace fconvert

#endif // JSON_H
