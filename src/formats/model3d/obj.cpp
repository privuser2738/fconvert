/**
 * OBJ (Wavefront) 3D model format implementation
 */

#include "obj.h"
#include <sstream>
#include <algorithm>
#include <cstring>

namespace fconvert {
namespace formats {

bool OBJCodec::is_obj(const uint8_t* data, size_t size) {
    if (size < 2) return false;

    // Check for common OBJ file starting patterns
    std::string start(reinterpret_cast<const char*>(data), std::min(size, size_t(100)));

    return (start.find("v ") != std::string::npos ||
            start.find("vn ") != std::string::npos ||
            start.find("vt ") != std::string::npos ||
            start.find("f ") != std::string::npos);
}

bool OBJCodec::parse_face_vertex(
    const std::string& token,
    int& vertex_index,
    int& texcoord_index,
    int& normal_index) {

    vertex_index = -1;
    texcoord_index = -1;
    normal_index = -1;

    size_t first_slash = token.find('/');
    if (first_slash == std::string::npos) {
        // Format: v
        vertex_index = std::stoi(token);
        return true;
    }

    size_t second_slash = token.find('/', first_slash + 1);

    // Parse vertex index
    vertex_index = std::stoi(token.substr(0, first_slash));

    if (second_slash == std::string::npos) {
        // Format: v/vt
        if (first_slash + 1 < token.length()) {
            texcoord_index = std::stoi(token.substr(first_slash + 1));
        }
    } else {
        // Format: v/vt/vn or v//vn
        if (second_slash > first_slash + 1) {
            texcoord_index = std::stoi(token.substr(first_slash + 1, second_slash - first_slash - 1));
        }
        if (second_slash + 1 < token.length()) {
            normal_index = std::stoi(token.substr(second_slash + 1));
        }
    }

    return true;
}

void OBJCodec::parse_line(
    const std::string& line,
    std::vector<Vec3>& vertices,
    std::vector<Vec3>& normals,
    std::vector<Triangle>& triangles) {

    std::istringstream iss(line);
    std::string type;
    iss >> type;

    if (type == "v") {
        // Vertex
        Vec3 v;
        iss >> v.x >> v.y >> v.z;
        vertices.push_back(v);
    }
    else if (type == "vn") {
        // Normal
        Vec3 n;
        iss >> n.x >> n.y >> n.z;
        normals.push_back(n);
    }
    else if (type == "f") {
        // Face - read all vertices
        std::vector<int> face_vertices;
        std::vector<int> face_normals;

        std::string token;
        while (iss >> token) {
            int v_idx, vt_idx, vn_idx;
            if (parse_face_vertex(token, v_idx, vt_idx, vn_idx)) {
                // OBJ indices are 1-based, convert to 0-based
                if (v_idx > 0) {
                    face_vertices.push_back(v_idx - 1);
                } else if (v_idx < 0) {
                    // Negative indices count from the end
                    face_vertices.push_back(vertices.size() + v_idx);
                }

                if (vn_idx > 0) {
                    face_normals.push_back(vn_idx - 1);
                } else if (vn_idx < 0) {
                    face_normals.push_back(normals.size() + vn_idx);
                }
            }
        }

        // Triangulate face (simple fan triangulation)
        if (face_vertices.size() >= 3) {
            for (size_t i = 1; i + 1 < face_vertices.size(); i++) {
                Triangle tri;

                // Set vertices
                int idx0 = face_vertices[0];
                int idx1 = face_vertices[i];
                int idx2 = face_vertices[i + 1];

                if (idx0 < (int)vertices.size() && idx1 < (int)vertices.size() && idx2 < (int)vertices.size()) {
                    tri.vertices[0] = vertices[idx0];
                    tri.vertices[1] = vertices[idx1];
                    tri.vertices[2] = vertices[idx2];

                    // Calculate or use provided normal
                    if (!face_normals.empty() && face_normals[0] < (int)normals.size()) {
                        tri.normal = normals[face_normals[0]];
                    } else {
                        tri.normal = STLCodec::calculate_normal(tri.vertices[0], tri.vertices[1], tri.vertices[2]);
                    }

                    triangles.push_back(tri);
                }
            }
        }
    }
}

fconvert_error_t OBJCodec::decode(
    const std::vector<uint8_t>& data,
    Mesh3D& mesh) {

    std::string content(reinterpret_cast<const char*>(data.data()), data.size());
    std::istringstream iss(content);

    std::vector<Vec3> vertices;
    std::vector<Vec3> normals;

    mesh.name = "mesh";
    mesh.triangles.clear();

    std::string line;
    while (std::getline(iss, line)) {
        // Skip empty lines and comments
        if (line.empty() || line[0] == '#') {
            continue;
        }

        // Handle object names
        if (line.size() > 2 && line[0] == 'o' && line[1] == ' ') {
            mesh.name = line.substr(2);
            continue;
        }

        parse_line(line, vertices, normals, mesh.triangles);
    }

    return FCONVERT_OK;
}

fconvert_error_t OBJCodec::encode(
    const Mesh3D& mesh,
    std::vector<uint8_t>& data) {

    data.clear();
    std::ostringstream oss;

    // Write header comment
    oss << "# Wavefront OBJ file\n";
    oss << "# Generated by fconvert\n";
    oss << "o " << (mesh.name.empty() ? "mesh" : mesh.name) << "\n\n";

    // Build unique vertex list
    std::vector<Vec3> unique_vertices;
    std::vector<Vec3> unique_normals;
    std::vector<int> vertex_indices;
    std::vector<int> normal_indices;

    for (const auto& tri : mesh.triangles) {
        // Add vertices and track indices
        for (int v = 0; v < 3; v++) {
            // Find or add vertex
            int v_idx = -1;
            for (size_t i = 0; i < unique_vertices.size(); i++) {
                const Vec3& existing = unique_vertices[i];
                if (std::abs(existing.x - tri.vertices[v].x) < 0.00001f &&
                    std::abs(existing.y - tri.vertices[v].y) < 0.00001f &&
                    std::abs(existing.z - tri.vertices[v].z) < 0.00001f) {
                    v_idx = i;
                    break;
                }
            }
            if (v_idx == -1) {
                v_idx = unique_vertices.size();
                unique_vertices.push_back(tri.vertices[v]);
            }
            vertex_indices.push_back(v_idx);
        }

        // Find or add normal
        int n_idx = -1;
        for (size_t i = 0; i < unique_normals.size(); i++) {
            const Vec3& existing = unique_normals[i];
            if (std::abs(existing.x - tri.normal.x) < 0.00001f &&
                std::abs(existing.y - tri.normal.y) < 0.00001f &&
                std::abs(existing.z - tri.normal.z) < 0.00001f) {
                n_idx = i;
                break;
            }
        }
        if (n_idx == -1) {
            n_idx = unique_normals.size();
            unique_normals.push_back(tri.normal);
        }
        normal_indices.push_back(n_idx);
    }

    // Write vertices
    for (const auto& v : unique_vertices) {
        oss << "v " << v.x << " " << v.y << " " << v.z << "\n";
    }
    oss << "\n";

    // Write normals
    for (const auto& n : unique_normals) {
        oss << "vn " << n.x << " " << n.y << " " << n.z << "\n";
    }
    oss << "\n";

    // Write faces
    for (size_t i = 0; i < mesh.triangles.size(); i++) {
        int v0 = vertex_indices[i * 3 + 0] + 1;  // OBJ is 1-based
        int v1 = vertex_indices[i * 3 + 1] + 1;
        int v2 = vertex_indices[i * 3 + 2] + 1;
        int n = normal_indices[i] + 1;

        oss << "f " << v0 << "//" << n << " "
                    << v1 << "//" << n << " "
                    << v2 << "//" << n << "\n";
    }

    // Convert to bytes
    std::string str = oss.str();
    data.assign(str.begin(), str.end());

    return FCONVERT_OK;
}

} // namespace formats
} // namespace fconvert
