#ifndef SPREADSHEET_CONVERTER_H
#define SPREADSHEET_CONVERTER_H
#include "../../core/converter.h"
namespace fconvert { namespace formats { class SpreadsheetConverter : public core::Converter { public: virtual fconvert_error_t convert(const std::vector<uint8_t>&, const std::string&, std::vector<uint8_t>&, const std::string&, const core::ConversionParams&) override { return FCONVERT_ERROR_UNSUPPORTED_CONVERSION; } virtual bool can_convert(const std::string&, const std::string&) override { return false; } virtual file_type_category_t get_category() const override { return FILE_TYPE_UNKNOWN; } }; }}
#endif
