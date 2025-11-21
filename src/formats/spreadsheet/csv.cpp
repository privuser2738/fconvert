/**
 * CSV (Comma-Separated Values) format implementation
 */

#include "csv.h"
#include <sstream>
#include <algorithm>

namespace fconvert {
namespace formats {

bool CSVCodec::is_csv(const uint8_t* data, size_t size) {
    if (size < 2) return false;

    // Check for common CSV patterns
    size_t check_size = std::min(size, size_t(1024));
    int commas = 0, semicolons = 0, tabs = 0, newlines = 0;

    for (size_t i = 0; i < check_size; i++) {
        switch (data[i]) {
            case ',': commas++; break;
            case ';': semicolons++; break;
            case '\t': tabs++; break;
            case '\n': newlines++; break;
        }
    }

    // Must have at least one delimiter and one newline
    return newlines > 0 && (commas > 0 || semicolons > 0 || tabs > 0);
}

char CSVCodec::detect_delimiter(const uint8_t* data, size_t size) {
    size_t check_size = std::min(size, size_t(1024));
    int commas = 0, semicolons = 0, tabs = 0;

    for (size_t i = 0; i < check_size; i++) {
        switch (data[i]) {
            case ',': commas++; break;
            case ';': semicolons++; break;
            case '\t': tabs++; break;
        }
    }

    // Return the most common delimiter
    if (tabs > commas && tabs > semicolons) return '\t';
    if (semicolons > commas) return ';';
    return ',';
}

std::vector<std::string> CSVCodec::parse_line(const std::string& line, char delimiter) {
    std::vector<std::string> fields;
    std::string field;
    bool in_quotes = false;
    bool prev_quote = false;

    for (size_t i = 0; i < line.size(); i++) {
        char c = line[i];

        if (in_quotes) {
            if (c == '"') {
                if (i + 1 < line.size() && line[i + 1] == '"') {
                    // Escaped quote
                    field += '"';
                    i++;
                } else {
                    // End of quoted field
                    in_quotes = false;
                    prev_quote = true;
                }
            } else {
                field += c;
            }
        } else {
            if (c == '"' && field.empty() && !prev_quote) {
                // Start of quoted field
                in_quotes = true;
            } else if (c == delimiter) {
                // Field separator
                fields.push_back(field);
                field.clear();
                prev_quote = false;
            } else if (c != '\r') {
                field += c;
            }
        }
    }

    // Add last field
    fields.push_back(field);

    return fields;
}

bool CSVCodec::needs_quoting(const std::string& field, char delimiter) {
    for (char c : field) {
        if (c == delimiter || c == '"' || c == '\n' || c == '\r') {
            return true;
        }
    }
    return false;
}

std::string CSVCodec::escape_field(const std::string& field, char delimiter) {
    if (!needs_quoting(field, delimiter)) {
        return field;
    }

    std::string result = "\"";
    for (char c : field) {
        if (c == '"') {
            result += "\"\"";  // Escape quote
        } else {
            result += c;
        }
    }
    result += "\"";
    return result;
}

fconvert_error_t CSVCodec::decode(
    const std::vector<uint8_t>& data,
    SpreadsheetData& sheet,
    char delimiter) {

    if (data.empty()) {
        return FCONVERT_ERROR_INVALID_PARAMETER;
    }

    sheet.rows.clear();
    sheet.delimiter = delimiter;

    // Auto-detect delimiter if default
    if (delimiter == ',') {
        delimiter = detect_delimiter(data.data(), data.size());
        sheet.delimiter = delimiter;
    }

    // Convert to string
    std::string content(reinterpret_cast<const char*>(data.data()), data.size());

    // Parse line by line
    std::istringstream iss(content);
    std::string line;

    while (std::getline(iss, line)) {
        // Remove trailing CR if present
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }

        std::vector<std::string> fields = parse_line(line, delimiter);
        sheet.rows.push_back(fields);
    }

    return FCONVERT_OK;
}

fconvert_error_t CSVCodec::encode(
    const SpreadsheetData& sheet,
    std::vector<uint8_t>& data,
    char delimiter) {

    std::ostringstream oss;

    for (size_t row = 0; row < sheet.rows.size(); row++) {
        const auto& fields = sheet.rows[row];

        for (size_t col = 0; col < fields.size(); col++) {
            if (col > 0) {
                oss << delimiter;
            }
            oss << escape_field(fields[col], delimiter);
        }
        oss << "\n";
    }

    std::string result = oss.str();
    data.assign(result.begin(), result.end());

    return FCONVERT_OK;
}

} // namespace formats
} // namespace fconvert
