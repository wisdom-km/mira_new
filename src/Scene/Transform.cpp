// Transform: Implementation for the DirectorDesk Scene module.
// This file owns project behavior only; keep platform and dependency boundaries explicit.

#include "DirectorDesk/Scene/Transform.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

namespace DirectorDesk::Scene {

glm::mat4 Transform::ToMatrix() const {
    const glm::mat4 translation = glm::translate(glm::mat4(1.0f), position);
    const glm::mat4 rotationMatrix = glm::mat4_cast(rotation);
    const glm::mat4 scaleMatrix = glm::scale(glm::mat4(1.0f), scale);
    return translation * rotationMatrix * scaleMatrix;
}

glm::vec3 Transform::EulerDegrees() const {
    const glm::vec3 radians = glm::eulerAngles(rotation);
    return glm::degrees(radians);
}

void Transform::SetEulerDegrees(const glm::vec3& eulerDegrees) {
    rotation = glm::quat(glm::radians(eulerDegrees));
}

} // namespace DirectorDesk::Scene
