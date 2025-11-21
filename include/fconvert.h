/**
 * fconvert - Enterprise-grade cross-platform file conversion tool
 * Copyright (c) 2025
 */

#ifndef FCONVERT_H
#define FCONVERT_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define FCONVERT_VERSION_MAJOR 1
#define FCONVERT_VERSION_MINOR 0
#define FCONVERT_VERSION_PATCH 0

// Error codes
typedef enum {
    FCONVERT_OK = 0,
    FCONVERT_ERROR_FILE_NOT_FOUND = -1,
    FCONVERT_ERROR_INVALID_FORMAT = -2,
    FCONVERT_ERROR_UNSUPPORTED_CONVERSION = -3,
    FCONVERT_ERROR_MEMORY = -4,
    FCONVERT_ERROR_IO = -5,
    FCONVERT_ERROR_CORRUPTED_FILE = -6,
    FCONVERT_ERROR_ENCODING = -7,
    FCONVERT_ERROR_DECODING = -8,
    FCONVERT_ERROR_INVALID_PARAMETER = -9,
    FCONVERT_ERROR_INVALID_ARGUMENT = -10
} fconvert_error_t;

// File type categories
typedef enum {
    FILE_TYPE_UNKNOWN = 0,
    FILE_TYPE_IMAGE,
    FILE_TYPE_AUDIO,
    FILE_TYPE_VIDEO,
    FILE_TYPE_MODEL3D,
    FILE_TYPE_ARCHIVE,
    FILE_TYPE_DOCUMENT,
    FILE_TYPE_SPREADSHEET,
    FILE_TYPE_VECTOR,
    FILE_TYPE_FONT,
    FILE_TYPE_CAD,
    FILE_TYPE_EBOOK,
    FILE_TYPE_SUBTITLE,
    FILE_TYPE_PRESENTATION,
    FILE_TYPE_DATA,
    FILE_TYPE_GIS,
    FILE_TYPE_SCIENTIFIC
} file_type_category_t;

// Conversion quality presets
typedef enum {
    QUALITY_LOW = 0,
    QUALITY_MEDIUM = 1,
    QUALITY_HIGH = 2,
    QUALITY_LOSSLESS = 3
} quality_preset_t;

// Generic buffer structure
typedef struct {
    uint8_t* data;
    size_t size;
    size_t capacity;
} buffer_t;

// Progress callback
typedef void (*progress_callback_t)(float progress, const char* status, void* user_data);

#ifdef __cplusplus
}
#endif

#endif // FCONVERT_H
