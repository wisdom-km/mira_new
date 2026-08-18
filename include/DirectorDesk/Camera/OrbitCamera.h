// OrbitCamera: Public or internal interface for the DirectorDesk Camera module.
// This file owns project behavior only; keep platform and dependency boundaries explicit.

#pragma once

#include "DirectorDesk/Camera/Presets.h"
#include "DirectorDesk/Renderer/Types.h"

#include <glm/vec3.hpp>

namespace DirectorDesk::Camera {

class OrbitCamera {
public:
    /// Rotates the camera around its target and clamps pitch to avoid pole singularities.
    void Rotate(float deltaYawDegrees, float deltaPitchDegrees);
    /// Moves the target in camera-relative screen space; deltas are pixel-like input units.
    void Pan(float deltaX, float deltaY);
    /// Applies exponential wheel zoom while keeping the camera distance within safe limits.
    void Zoom(float wheelDelta);

    /// Builds the view and right-handed zero-to-one projection matrices for a viewport.
    [[nodiscard]] Renderer::CameraView BuildView(float aspectRatio) const;
    /// Returns the world-space camera position derived from the orbit parameters.
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

    /// Sets the point around which the camera orbits.
    void SetTarget(const glm::vec3& target) {
        m_target = target;
    }
    /// Sets and clamps the orbit distance.
    void SetDistance(float distance);
    void SetYawDegrees(float yawDegrees) {
        m_yawDegrees = yawDegrees;
    }
    /// Sets and clamps the vertical orbit angle.
    void SetPitchDegrees(float pitchDegrees);
    /// Sets and clamps the vertical field of view in degrees.
    void SetFovYDegrees(float fovYDegrees);
    /// Applies a persisted preset pose to the camera.
    void ApplyPose(const CameraPose& pose);
    /// Restores persisted camera values, deriving orbit angles when old data lacks them.
    bool Restore(const glm::vec3& target, const glm::vec3& position, float fovYDegrees,
                 float nearPlane, float farPlane, float distance, float yawDegrees,
                 float pitchDegrees, bool hasOrbitNumbers);

    [[nodiscard]] float FovYDegrees() const {
        return m_fovYDegrees;
    }
    [[nodiscard]] float NearPlane() const {
        return m_nearPlane;
    }
    [[nodiscard]] float FarPlane() const {
        return m_farPlane;
    }

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
