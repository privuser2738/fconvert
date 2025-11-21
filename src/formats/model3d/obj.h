/**
 * OBJ (Wavefront) 3D model format
 * Text-based format widely supported in 3D applications
 */

#ifndef OBJ_H
#define OBJ_H

#include "../../../include/fconvert.h"
#include "stl.h"  // Reuse Vec3 and Mesh3D
#include <vector>
#include <string>

namespace fconvert {
namespace formats {

// OBJ codec
class OBJCodec {
public:
    /**
     * Decode OBJ file
     */
    static fconvert_error_t decode(
        const std::vector<uint8_t>& data,
        Mesh3D& mesh);

    /**
     * Encode to OBJ format
     */
    static fconvert_error_t encode(
        const Mesh3D& mesh,
        std::vector<uint8_t>& data);

    /**
     * Check if data is OBJ format
     */
    static bool is_obj(const uint8_t* data, size_t size);

private:
    // Parse a single line
    static void parse_line(
        const std::string& line,
        std::vector<Vec3>& vertices,
        std::vector<Vec3>& normals,
        std::vector<Triangle>& triangles);

    // Parse face indices (handles "v", "v/vt", "v/vt/vn", "v//vn" formats)
    static bool parse_face_vertex(
        const std::string& token,
        int& vertex_index,
        int& texcoord_index,
        int& normal_index);
};

} // namespace formats
} // namespace fconvert

#endif // OBJ_H
