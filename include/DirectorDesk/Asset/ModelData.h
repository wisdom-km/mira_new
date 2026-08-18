// ModelData: Public or internal interface for the DirectorDesk Asset module.
// This file owns project behavior only; keep platform and dependency boundaries explicit.

#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

namespace DirectorDesk::Asset {

struct Vertex {
    glm::vec3 position{0.0f, 0.0f, 0.0f};
    glm::vec3 normal{0.0f, 1.0f, 0.0f};
    glm::vec2 uv{0.0f, 0.0f};
    std::uint32_t abgr = 0xffffffffu;
};

struct Material {
    glm::vec4 baseColor{1.0f, 1.0f, 1.0f, 1.0f};
    std::uint32_t textureWidth = 0;
    std::uint32_t textureHeight = 0;
    std::vector<std::uint8_t> rgba;
};

struct Primitive {
    std::vector<Vertex> vertices;
    std::vector<std::uint32_t> indices;
    std::uint32_t materialIndex = 0;
    glm::mat4 localTransform{1.0f};
};

struct ModelData {
    std::string sourcePath;
    std::string name;
    std::vector<Material> materials;
    std::vector<Primitive> primitives;
    std::vector<std::string> warnings;
};

} // namespace DirectorDesk::Asset
