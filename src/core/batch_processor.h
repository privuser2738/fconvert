/**
 * Batch file processing
 */

#ifndef BATCH_PROCESSOR_H
#define BATCH_PROCESSOR_H

#include "converter.h"
#include <string>
#include <vector>

namespace fconvert {
namespace core {

struct BatchResult {
    int total_files = 0;
    int successful = 0;
    int failed = 0;
    std::vector<std::string> failed_files;
};

class BatchProcessor {
public:
    BatchProcessor();
    ~BatchProcessor();

    BatchResult process_files(
        const std::vector<std::string>& input_files,
        const std::string& output_format,
        const std::string& output_folder,
        const ConversionParams& params);

    BatchResult process_folder(
        const std::string& input_folder,
        const std::string& output_format,
        const std::string& output_folder,
        bool recursive,
        const ConversionParams& params);

    void set_skip_errors(bool skip) { skip_errors_ = skip; }
    void set_overwrite(bool overwrite) { overwrite_ = overwrite; }

private:
    bool skip_errors_;
    bool overwrite_;

    fconvert_error_t process_single_file(
        const std::string& input_path,
        const std::string& output_path,
        const ConversionParams& params);
};

} // namespace core
} // namespace fconvert

#endif // BATCH_PROCESSOR_H
