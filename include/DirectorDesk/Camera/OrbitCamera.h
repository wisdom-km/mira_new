#pragma once

#include "DirectorDesk/Renderer/Types.h"

#include <glm/vec3.hpp>

namespace DirectorDesk::Camera {

class OrbitCamera {
public:
    void Rotate(float deltaYawDegrees, float deltaPitchDegrees);
    void Pan(float deltaX, float deltaY);
    void Zoom(float wheelDelta);

    [[nodiscard]] Renderer::CameraView BuildView(float aspectRatio) const;
    [[nodiscard]] glm::vec3 Position() const;
    [[nodiscard]] glm::vec3 Target() const {
        return m_target;
    }
    [[nodiscard]] float Distance() const {
        return m_distance;
    }
    [[nodiscard]] float YawDegrees() const {
        return m_yawDegrees;
    }
    [[nodiscard]] float PitchDegrees() const {
        return m_pitchDegrees;
    }

    void SetTarget(const glm::vec3& target) {
        m_target = target;
    }
    void SetDistance(float distance);

private:
    glm::vec3 m_target{0.0f, 0.5f, 0.0f};
    float m_distance = 6.0f;
    float m_yawDegrees = 40.0f;
    float m_pitchDegrees = 25.0f;
    float m_fovYDegrees = 50.0f;
    float m_nearPlane = 0.1f;
    float m_farPlane = 200.0f;
};

} // namespace DirectorDesk::Camera
