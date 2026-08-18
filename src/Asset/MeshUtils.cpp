#include "MeshUtils.h"

#include <cmath>

namespace DirectorDesk::Asset {

void GenerateNormals(std::vector<Vertex>& vertices, const std::vector<std::uint32_t>& indices) {
    for (Vertex& vertex : vertices) {
        vertex.normal = glm::vec3(0.0f);
    }

    const std::size_t triangleCount = indices.size() / 3;
    for (std::size_t i = 0; i < triangleCount; ++i) {
        const std::uint32_t i0 = indices[i * 3];
        const std::uint32_t i1 = indices[i * 3 + 1];
        const std::uint32_t i2 = indices[i * 3 + 2];
        if (i0 >= vertices.size() || i1 >= vertices.size() || i2 >= vertices.size()) {
            continue;
        }
        const glm::vec3 edge1 = vertices[i1].position - vertices[i0].position;
        const glm::vec3 edge2 = vertices[i2].position - vertices[i0].position;
        const glm::vec3 face = glm::cross(edge1, edge2);
        vertices[i0].normal += face;
        vertices[i1].normal += face;
        vertices[i2].normal += face;
    }

    for (Vertex& vertex : vertices) {
        const float length = glm::length(vertex.normal);
        vertex.normal = length > 1.0e-8f ? vertex.normal / length : glm::vec3(0.0f, 1.0f, 0.0f);
    }
}

} // namespace DirectorDesk::Asset
