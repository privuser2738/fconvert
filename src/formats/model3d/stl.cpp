/**
 * STL (Stereolithography) 3D model format implementation
 */

#include "stl.h"
#include <cstring>
#include <cmath>
#include <sstream>
#include <algorithm>

namespace fconvert {
namespace formats {

// Vector math operations
Vec3 STLCodec::cross_product(const Vec3& a, const Vec3& b) {
    return Vec3(
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x
    );
}

float STLCodec::dot_product(const Vec3& a, const Vec3& b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

Vec3 STLCodec::normalize(const Vec3& v) {
    float length = std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
    if (length < 0.000001f) {
        return Vec3(0, 0, 0);
    }
    return Vec3(v.x / length, v.y / length, v.z / length);
}

Vec3 STLCodec::calculate_normal(const Vec3& v1, const Vec3& v2, const Vec3& v3) {
    // Calculate two edge vectors
    Vec3 edge1(v2.x - v1.x, v2.y - v1.y, v2.z - v1.z);
    Vec3 edge2(v3.x - v1.x, v3.y - v1.y, v3.z - v1.z);

    // Cross product gives normal
    Vec3 normal = cross_product(edge1, edge2);
    return normalize(normal);
}

bool STLCodec::is_stl(const uint8_t* data, size_t size) {
    if (size < 6) return false;

    // Check for ASCII format (starts with "solid")
    if (size >= 5 && std::memcmp(data, "solid", 5) == 0) {
        return true;
    }

    // Check for binary format (at least 84 bytes header + one triangle)
    if (size >= 84) {
        // Read triangle count from bytes 80-83
        uint32_t num_triangles;
        std::memcpy(&num_triangles, data + 80, sizeof(uint32_t));

        // Each triangle is 50 bytes
        size_t expected_size = 84 + num_triangles * 50;
        if (size == expected_size) {
            return true;
        }
    }

    return false;
}

fconvert_error_t STLCodec::decode_ascii(
    const char* text,
    size_t size,
    Mesh3D& mesh) {

    std::string content(text, size);
    std::istringstream iss(content);
    std::string line, word;

    // Read "solid" header
    std::getline(iss, line);
    std::istringstream headerss(line);
    headerss >> word;

    if (word != "solid") {
        return FCONVERT_ERROR_INVALID_FORMAT;
    }

    // Read optional name
    if (headerss >> word) {
        mesh.name = word;
    }

    mesh.triangles.clear();

    // Parse triangles
    while (std::getline(iss, line)) {
        std::istringstream liness(line);
        liness >> word;

        if (word == "facet") {
            Triangle tri;

            // Read normal
            liness >> word; // "normal"
            liness >> tri.normal.x >> tri.normal.y >> tri.normal.z;

            // Read "outer loop"
            std::getline(iss, line);

            // Read three vertices
            for (int i = 0; i < 3; i++) {
                std::getline(iss, line);
                std::istringstream vertexss(line);
                vertexss >> word; // "vertex"
                vertexss >> tri.vertices[i].x >> tri.vertices[i].y >> tri.vertices[i].z;
            }

            // Read "endloop" and "endfacet"
            std::getline(iss, line); // endloop
            std::getline(iss, line); // endfacet

            mesh.triangles.push_back(tri);

        } else if (word == "endsolid") {
            break;
        }
    }

    return FCONVERT_OK;
}

fconvert_error_t STLCodec::decode_binary(
    const uint8_t* data,
    size_t size,
    Mesh3D& mesh) {

    if (size < 84) {
        return FCONVERT_ERROR_INVALID_FORMAT;
    }

    // Read header (80 bytes) - usually ignored or contains model name
    mesh.name = std::string(reinterpret_cast<const char*>(data),
                           strnlen(reinterpret_cast<const char*>(data), 80));

    // Read number of triangles
    uint32_t num_triangles;
    std::memcpy(&num_triangles, data + 80, sizeof(uint32_t));

    // Validate size
    size_t expected_size = 84 + num_triangles * 50;
    if (size < expected_size) {
        return FCONVERT_ERROR_CORRUPTED_FILE;
    }

    mesh.triangles.reserve(num_triangles);

    // Read triangles
    const uint8_t* ptr = data + 84;
    for (uint32_t i = 0; i < num_triangles; i++) {
        Triangle tri;

        // Read normal (3 floats = 12 bytes)
        std::memcpy(&tri.normal.x, ptr, sizeof(float)); ptr += sizeof(float);
        std::memcpy(&tri.normal.y, ptr, sizeof(float)); ptr += sizeof(float);
        std::memcpy(&tri.normal.z, ptr, sizeof(float)); ptr += sizeof(float);

        // Read three vertices (9 floats = 36 bytes)
        for (int v = 0; v < 3; v++) {
            std::memcpy(&tri.vertices[v].x, ptr, sizeof(float)); ptr += sizeof(float);
            std::memcpy(&tri.vertices[v].y, ptr, sizeof(float)); ptr += sizeof(float);
            std::memcpy(&tri.vertices[v].z, ptr, sizeof(float)); ptr += sizeof(float);
        }

        // Skip attribute byte count (2 bytes)
        ptr += 2;

        mesh.triangles.push_back(tri);
    }

    return FCONVERT_OK;
}

fconvert_error_t STLCodec::decode(
    const std::vector<uint8_t>& data,
    Mesh3D& mesh) {

    if (data.empty()) {
        return FCONVERT_ERROR_INVALID_FORMAT;
    }

    // Check if ASCII format
    if (data.size() >= 5 && std::memcmp(data.data(), "solid", 5) == 0) {
        // Verify it's actually ASCII (not binary with "solid" in header)
        bool is_ascii = false;
        for (size_t i = 5; i < std::min(data.size(), size_t(100)); i++) {
            if (data[i] == '\n') {
                is_ascii = true;
                break;
            }
            if (data[i] < 32 && data[i] != '\t' && data[i] != '\r') {
                is_ascii = false;
                break;
            }
        }

        if (is_ascii) {
            return decode_ascii(reinterpret_cast<const char*>(data.data()), data.size(), mesh);
        }
    }

    // Try binary format
    return decode_binary(data.data(), data.size(), mesh);
}

fconvert_error_t STLCodec::encode_binary(
    const Mesh3D& mesh,
    std::vector<uint8_t>& data) {

    data.clear();
    data.reserve(84 + mesh.triangles.size() * 50);

    // Write header (80 bytes)
    char header[80];
    std::memset(header, 0, 80);
    if (!mesh.name.empty()) {
        size_t len = std::min(mesh.name.length(), size_t(79));
        std::memcpy(header, mesh.name.c_str(), len);
    } else {
        std::strcpy(header, "Binary STL");
    }
    data.insert(data.end(), header, header + 80);

    // Write number of triangles
    uint32_t num_triangles = mesh.triangles.size();
    const uint8_t* num_ptr = reinterpret_cast<const uint8_t*>(&num_triangles);
    data.insert(data.end(), num_ptr, num_ptr + sizeof(uint32_t));

    // Write triangles
    for (const auto& tri : mesh.triangles) {
        // Write normal
        const uint8_t* ptr;
        ptr = reinterpret_cast<const uint8_t*>(&tri.normal.x);
        data.insert(data.end(), ptr, ptr + sizeof(float));
        ptr = reinterpret_cast<const uint8_t*>(&tri.normal.y);
        data.insert(data.end(), ptr, ptr + sizeof(float));
        ptr = reinterpret_cast<const uint8_t*>(&tri.normal.z);
        data.insert(data.end(), ptr, ptr + sizeof(float));

        // Write three vertices
        for (int v = 0; v < 3; v++) {
            ptr = reinterpret_cast<const uint8_t*>(&tri.vertices[v].x);
            data.insert(data.end(), ptr, ptr + sizeof(float));
            ptr = reinterpret_cast<const uint8_t*>(&tri.vertices[v].y);
            data.insert(data.end(), ptr, ptr + sizeof(float));
            ptr = reinterpret_cast<const uint8_t*>(&tri.vertices[v].z);
            data.insert(data.end(), ptr, ptr + sizeof(float));
        }

        // Write attribute byte count (always 0)
        data.push_back(0);
        data.push_back(0);
    }

    return FCONVERT_OK;
}

fconvert_error_t STLCodec::encode_ascii(
    const Mesh3D& mesh,
    std::vector<uint8_t>& data) {

    data.clear();
    std::ostringstream oss;

    // Write header
    oss << "solid " << (mesh.name.empty() ? "mesh" : mesh.name) << "\n";

    // Write triangles
    for (const auto& tri : mesh.triangles) {
        oss << "  facet normal "
            << tri.normal.x << " " << tri.normal.y << " " << tri.normal.z << "\n";
        oss << "    outer loop\n";

        for (int v = 0; v < 3; v++) {
            oss << "      vertex "
                << tri.vertices[v].x << " "
                << tri.vertices[v].y << " "
                << tri.vertices[v].z << "\n";
        }

        oss << "    endloop\n";
        oss << "  endfacet\n";
    }

    // Write footer
    oss << "endsolid " << (mesh.name.empty() ? "mesh" : mesh.name) << "\n";

    // Convert to bytes
    std::string str = oss.str();
    data.assign(str.begin(), str.end());

    return FCONVERT_OK;
}

} // namespace formats
} // namespace fconvert
