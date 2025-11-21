cmake_minimum_required(VERSION 3.15)
project(fconvert VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Platform detection
if(WIN32)
    add_definitions(-DPLATFORM_WINDOWS)
elseif(UNIX AND NOT APPLE)
    add_definitions(-DPLATFORM_LINUX)
elseif(APPLE)
    add_definitions(-DPLATFORM_MACOS)
endif()

# Compiler flags
if(MSVC)
    add_compile_options(/W4 /WX-)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Source files
set(SOURCES
    src/main.cpp
    src/cli/argument_parser.cpp
    src/cli/config.cpp
    src/core/converter.cpp
    src/core/file_detector.cpp
    src/core/batch_processor.cpp
    src/core/logger.cpp
    src/utils/file_utils.cpp
    src/utils/memory.cpp
    src/utils/math_utils.cpp
    src/utils/crc32.cpp
    src/utils/deflate.cpp
    src/utils/deflate_compress.cpp
    src/utils/lz77.cpp
    src/utils/image_transform.cpp
    src/utils/gzip.cpp
    src/utils/tar.cpp
    src/utils/zip.cpp

    # Image formats
    src/formats/image/bmp.cpp
    src/formats/image/png.cpp
    src/formats/image/jpeg.cpp
    src/formats/image/tga.cpp
    src/formats/image/netpbm.cpp
    src/formats/image/image_converter.cpp

    # Audio formats
    src/formats/audio/wav.cpp
    src/formats/audio/mp3.cpp
    src/formats/audio/ogg.cpp
    src/formats/audio/flac.cpp
    src/formats/audio/aac.cpp
    src/formats/audio/audio_converter.cpp

    # Video formats
    src/formats/video/mp4.cpp
    src/formats/video/avi.cpp
    src/formats/video/webm.cpp
    src/formats/video/video_converter.cpp

    # 3D Model formats
    src/formats/model3d/obj.cpp
    src/formats/model3d/stl.cpp
    src/formats/model3d/fbx.cpp
    src/formats/model3d/dae.cpp
    src/formats/model3d/model3d_converter.cpp

    # Archive formats
    src/formats/archive/zip.cpp
    src/formats/archive/tar.cpp
    src/formats/archive/gz.cpp
    src/formats/archive/bz2.cpp
    src/formats/archive/xz.cpp
    src/formats/archive/archive_converter.cpp

    # Document formats
    src/formats/document/pdf.cpp
    src/formats/document/docx.cpp
    src/formats/document/txt.cpp
    src/formats/document/rtf.cpp
    src/formats/document/html.cpp
    src/formats/document/markdown.cpp
    src/formats/document/document_converter.cpp

    # Spreadsheet formats
    src/formats/spreadsheet/xlsx.cpp
    src/formats/spreadsheet/csv.cpp
    src/formats/spreadsheet/ods.cpp
    src/formats/spreadsheet/spreadsheet_converter.cpp

    # Vector graphics
    src/formats/vector/svg.cpp
    src/formats/vector/eps.cpp
    src/formats/vector/vector_converter.cpp

    # Font formats
    src/formats/font/ttf.cpp
    src/formats/font/otf.cpp
    src/formats/font/woff.cpp
    src/formats/font/font_converter.cpp

    # Data formats
    src/formats/data/json.cpp
    src/formats/data/xml.cpp
    src/formats/data/yaml.cpp
    src/formats/data/toml.cpp
    src/formats/data/ini.cpp
    src/formats/data/data_converter.cpp

    # Disc image formats
    src/formats/disc/iso.cpp
    src/formats/disc/bincue.cpp
    src/formats/disc/vhd.cpp
    src/formats/disc/chd.cpp
    src/formats/disc/disc_converter.cpp

    # Filesystem formats
    src/formats/filesystem/fat32.cpp
    src/formats/filesystem/ext2.cpp
)

# Include directories
include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
)

# Executable
add_executable(fconvert ${SOURCES})

# Platform-specific linking
if(WIN32)
    target_link_libraries(fconvert)
elseif(UNIX)
    target_link_libraries(fconvert m pthread)
endif()

# Install target
install(TARGETS fconvert DESTINATION bin)
install(FILES config/default_config.ini DESTINATION share/fconvert)
