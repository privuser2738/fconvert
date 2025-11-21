/**
 * CSV (Comma-Separated Values) format
 */

#ifndef CSV_H
#define CSV_H

#include "../../../include/fconvert.h"
#include <vector>
#include <string>

namespace fconvert {
namespace formats {

// Spreadsheet data structure
struct SpreadsheetData {
    std::vector<std::vector<std::string>> rows;
    char delimiter;
    bool has_header;

    SpreadsheetData() : delimiter(','), has_header(false) {}
};

/**
 * CSV codec
 */
class CSVCodec {
public:
    /**
     * Decode CSV file to spreadsheet data
     */
    static fconvert_error_t decode(
        const std::vector<uint8_t>& data,
        SpreadsheetData& sheet,
        char delimiter = ',');

    /**
     * Encode spreadsheet data to CSV
     */
    static fconvert_error_t encode(
        const SpreadsheetData& sheet,
        std::vector<uint8_t>& data,
        char delimiter = ',');

    /**
     * Check if data is CSV format
     */
    static bool is_csv(const uint8_t* data, size_t size);

    /**
     * Detect delimiter (comma, semicolon, tab)
     */
    static char detect_delimiter(const uint8_t* data, size_t size);

private:
    // Parse a single CSV line
    static std::vector<std::string> parse_line(
        const std::string& line,
        char delimiter);

    // Escape a field for CSV output
    static std::string escape_field(
        const std::string& field,
        char delimiter);

    // Check if field needs quoting
    static bool needs_quoting(
        const std::string& field,
        char delimiter);
};

} // namespace formats
} // namespace fconvert

#endif // CSV_H
