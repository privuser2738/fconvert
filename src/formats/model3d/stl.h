/**
 * STL (Stereolithography) 3D model format
 * Simple triangle mesh format for 3D printing
 */

#ifndef STL_H
#define STL_H

#include "../../../include/fconvert.h"
#include <vector>
#include <string>
#include <cstdint>

namespace fconvert {
namespace formats {

// 3D vector/point
struct Vec3 {
    float x, y, z;

    Vec3() : x(0), y(0), z(0) {}
    Vec3(float _x, float _y, float _z) : x(_x), y(_y), z(_z) {}
};

// Triangle face with normal
struct Triangle {
    Vec3 normal;
    Vec3 vertices[3];
};

// 3D mesh data structure
struct Mesh3D {
    std::string name;
    std::vector<Triangle> triangles;
};

// STL codec
class STLCodec {
public:
    /**
     * Decode STL file (ASCII or Binary)
     */
    static fconvert_error_t decode(
        const std::vector<uint8_t>& data,
        Mesh3D& mesh);

    /**
     * Encode to STL binary format
     */
    static fconvert_error_t encode_binary(
        const Mesh3D& mesh,
        std::vector<uint8_t>& data);

    /**
     * Encode to STL ASCII format
     */
    static fconvert_error_t encode_ascii(
        const Mesh3D& mesh,
        std::vector<uint8_t>& data);

    /**
     * Check if data is STL format
     */
    static bool is_stl(const uint8_t* data, size_t size);

    // Calculate normal from triangle vertices (right-hand rule)
    static Vec3 calculate_normal(const Vec3& v1, const Vec3& v2, const Vec3& v3);

private:
    // Decode ASCII format
    static fconvert_error_t decode_ascii(
        const char* text,
        size_t size,
        Mesh3D& mesh);

    // Decode binary format
    static fconvert_error_t decode_binary(
        const uint8_t* data,
        size_t size,
        Mesh3D& mesh);

    // Vector operations
    static Vec3 cross_product(const Vec3& a, const Vec3& b);
    static Vec3 normalize(const Vec3& v);
    static float dot_product(const Vec3& a, const Vec3& b);
};

} // namespace formats
} // namespace fconvert

#endif // STL_H
