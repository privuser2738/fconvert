// Quick utility to create a test BMP file
#include <fstream>
#include <cstdint>
#include <cstring>

#pragma pack(push, 1)
struct BMPFileHeader {
    uint16_t signature;
    uint32_t file_size;
    uint16_t reserved1;
    uint16_t reserved2;
    uint32_t data_offset;
};

struct BMPInfoHeader {
    uint32_t header_size;
    int32_t width;
    int32_t height;
    uint16_t planes;
    uint16_t bits_per_pixel;
    uint32_t compression;
    uint32_t image_size;
    int32_t x_pixels_per_meter;
    int32_t y_pixels_per_meter;
    uint32_t colors_used;
    uint32_t colors_important;
};
#pragma pack(pop)

int main() {
    const int width = 64;
    const int height = 64;
    const int bytes_per_pixel = 3;
    const int row_size = ((width * bytes_per_pixel + 3) / 4) * 4;
    const int pixel_data_size = row_size * height;

    BMPFileHeader file_header = {};
    file_header.signature = 0x4D42;
    file_header.file_size = sizeof(BMPFileHeader) + sizeof(BMPInfoHeader) + pixel_data_size;
    file_header.data_offset = sizeof(BMPFileHeader) + sizeof(BMPInfoHeader);

    BMPInfoHeader info_header = {};
    info_header.header_size = sizeof(BMPInfoHeader);
    info_header.width = width;
    info_header.height = height;
    info_header.planes = 1;
    info_header.bits_per_pixel = 24;
    info_header.compression = 0;
    info_header.image_size = pixel_data_size;
    info_header.x_pixels_per_meter = 2835;
    info_header.y_pixels_per_meter = 2835;

    std::ofstream file("test_input.bmp", std::ios::binary);
    file.write(reinterpret_cast<char*>(&file_header), sizeof(file_header));
    file.write(reinterpret_cast<char*>(&info_header), sizeof(info_header));

    // Create a simple gradient pattern
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            uint8_t r = (x * 255) / width;
            uint8_t g = (y * 255) / height;
            uint8_t b = ((x + y) * 255) / (width + height);
            file.put(b);  // BMP is BGR
            file.put(g);
            file.put(r);
        }
        // Write padding
        for (int p = 0; p < row_size - (width * bytes_per_pixel); p++) {
            file.put(0);
        }
    }

    file.close();
    return 0;
}
