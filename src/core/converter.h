/**
 * Base converter interface and registry
 */

#ifndef CONVERTER_H
#define CONVERTER_H

#include "../../include/fconvert.h"
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <functional>

namespace fconvert {
namespace core {

struct ConversionParams {
    // Quality settings
    int quality = 85;
    bool lossless = false;

    // Image params
    int width = -1;
    int height = -1;
    bool keep_aspect_ratio = true;
    int rotate = 0;  // Rotation in degrees (0, 90, 180, 270)
    bool flip_horizontal = false;
    bool flip_vertical = false;
    int interpolation = 1;  // 0=nearest, 1=bilinear, 2=bicubic

    // Audio params
    int sample_rate = 44100;
    int bitrate = 192000;
    int channels = 2;

    // Video params
    int fps = 30;
    int video_bitrate = 2000000;
    std::string codec;

    // Progress callback
    progress_callback_t progress_callback = nullptr;
    void* progress_user_data = nullptr;
};

class Converter {
public:
    virtual ~Converter() = default;

    virtual fconvert_error_t convert(
        const std::vector<uint8_t>& input_data,
        const std::string& input_format,
        std::vector<uint8_t>& output_data,
        const std::string& output_format,
        const ConversionParams& params) = 0;

    virtual bool can_convert(const std::string& from_format, const std::string& to_format) = 0;
    virtual file_type_category_t get_category() const = 0;
};

class ConverterRegistry {
public:
    static ConverterRegistry& instance();

    void register_converter(std::shared_ptr<Converter> converter);

    fconvert_error_t convert_file(
        const std::string& input_path,
        const std::string& output_path,
        const ConversionParams& params = ConversionParams());

    fconvert_error_t convert_data(
        const std::vector<uint8_t>& input_data,
        const std::string& input_format,
        std::vector<uint8_t>& output_data,
        const std::string& output_format,
        const ConversionParams& params = ConversionParams());

    bool can_convert(const std::string& from_format, const std::string& to_format);

private:
    ConverterRegistry();
    ~ConverterRegistry();

    std::vector<std::shared_ptr<Converter>> converters_;
    std::shared_ptr<Converter> find_converter(const std::string& from, const std::string& to);
};

} // namespace core
} // namespace fconvert

#endif // CONVERTER_H
