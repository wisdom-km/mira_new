#pragma once

#include <glm/gtc/quaternion.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

namespace DirectorDesk::Scene {

struct Transform {
    glm::vec3 position{0.0f, 0.0f, 0.0f};
    glm::quat rotation{1.0f, 0.0f, 0.0f, 0.0f};
    glm::vec3 scale{1.0f, 1.0f, 1.0f};

    [[nodiscard]] glm::mat4 ToMatrix() const;
    [[nodiscard]] glm::vec3 EulerDegrees() const;
    void SetEulerDegrees(const glm::vec3& eulerDegrees);
};

} // namespace DirectorDesk::Scene
