/**
 * Archive format converter
 */

#ifndef ARCHIVE_CONVERTER_H
#define ARCHIVE_CONVERTER_H

#include "../../core/converter.h"
#include <vector>
#include <string>

namespace fconvert {
namespace formats {

class ArchiveConverter : public core::Converter {
public:
    ArchiveConverter();

    virtual fconvert_error_t convert(
        const std::vector<uint8_t>& input_data,
        const std::string& input_format,
        std::vector<uint8_t>& output_data,
        const std::string& output_format,
        const core::ConversionParams& params) override;

    virtual bool can_convert(const std::string& from_format, const std::string& to_format) override;
    virtual file_type_category_t get_category() const override { return FILE_TYPE_ARCHIVE; }

private:
    bool is_supported_format(const std::string& format) const;
};

} // namespace formats
} // namespace fconvert

#endif // ARCHIVE_CONVERTER_H
