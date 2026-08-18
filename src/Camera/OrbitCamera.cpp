// OrbitCamera: Implementation for the DirectorDesk Camera module.
// This file owns project behavior only; keep platform and dependency boundaries explicit.

#include "DirectorDesk/Camera/OrbitCamera.h"

#include <algorithm>
#include <cmath>

#include <glm/gtc/matrix_transform.hpp>

namespace DirectorDesk::Camera {
namespace {

constexpr float kMinDistance = 0.5f;
constexpr float kMaxDistance = 80.0f;
constexpr float kMinPitch = -89.0f;
constexpr float kMaxPitch = 89.0f;

glm::vec3 OffsetFromAngles(float distance, float yawDegrees, float pitchDegrees) {
    // The orbit convention is +Y up, with yaw measured around the vertical axis.
    const float yaw = glm::radians(yawDegrees);
    const float pitch = glm::radians(pitchDegrees);
    const float cosPitch = std::cos(pitch);
    return glm::vec3(distance * std::cos(yaw) * cosPitch, distance * std::sin(pitch),
                     distance * std::sin(yaw) * cosPitch);
}

} // namespace

void OrbitCamera::Rotate(float deltaYawDegrees, float deltaPitchDegrees) {
    m_yawDegrees += deltaYawDegrees;
    m_pitchDegrees = std::clamp(m_pitchDegrees + deltaPitchDegrees, kMinPitch, kMaxPitch);
}

void OrbitCamera::Pan(float deltaX, float deltaY) {
    const glm::vec3 offset = OffsetFromAngles(m_distance, m_yawDegrees, m_pitchDegrees);
    const glm::vec3 forward = glm::normalize(-offset);
    glm::vec3 right = glm::cross(forward, glm::vec3(0.0f, 1.0f, 0.0f));
    if (glm::dot(right, right) < 1.0e-8f) {
        right = glm::vec3(1.0f, 0.0f, 0.0f);
    } else {
        right = glm::normalize(right);
    }
    const glm::vec3 up = glm::normalize(glm::cross(right, forward));
    const float panScale = m_distance * 0.0025f;
    m_target += (-right * deltaX + up * deltaY) * panScale;
}

void OrbitCamera::Zoom(float wheelDelta) {
    // Exponential scaling makes equal wheel deltas feel consistent at every distance.
    SetDistance(m_distance * std::pow(0.9f, wheelDelta));
}

void OrbitCamera::SetDistance(float distance) {
    m_distance = std::clamp(distance, kMinDistance, kMaxDistance);
}

void OrbitCamera::SetPitchDegrees(float pitchDegrees) {
    m_pitchDegrees = std::clamp(pitchDegrees, kMinPitch, kMaxPitch);
}

void OrbitCamera::SetFovYDegrees(float fovYDegrees) {
    m_fovYDegrees = std::clamp(fovYDegrees, 20.0f, 90.0f);
}

void OrbitCamera::ApplyPose(const CameraPose& pose) {
    m_target = pose.target;
    SetDistance(pose.distance);
    SetYawDegrees(pose.yawDegrees);
    SetPitchDegrees(pose.pitchDegrees);
    SetFovYDegrees(pose.fovYDegrees);
}

bool OrbitCamera::Restore(const glm::vec3& target, const glm::vec3& position, float fovYDegrees,
                          float nearPlane, float farPlane, float distance, float yawDegrees,
                          float pitchDegrees, bool hasOrbitNumbers) {
    if (!std::isfinite(target.x) || !std::isfinite(target.y) || !std::isfinite(target.z) ||
        !std::isfinite(position.x) || !std::isfinite(position.y) || !std::isfinite(position.z) ||
        !std::isfinite(fovYDegrees) || !std::isfinite(nearPlane) || !std::isfinite(farPlane)) {
        return false;
    }
    if (nearPlane <= 0.0f || farPlane <= nearPlane || fovYDegrees <= 1.0f ||
        fovYDegrees >= 179.0f) {
        return false;
    }

    m_target = target;
    m_nearPlane = nearPlane;
    m_farPlane = farPlane;
    m_fovYDegrees = fovYDegrees;
    if (hasOrbitNumbers && std::isfinite(distance) && distance > 0.0f && std::isfinite(yawDegrees) &&
        std::isfinite(pitchDegrees)) {
        m_distance = distance;
        m_yawDegrees = yawDegrees;
        m_pitchDegrees = std::clamp(pitchDegrees, kMinPitch, kMaxPitch);
        return true;
    }

    // Older project files stored position only; derive orbit coordinates when needed.
    const glm::vec3 offset = position - target;
    const float length = glm::length(offset);
    if (!std::isfinite(length) || length < 1.0e-4f) {
        return false;
    }
    m_distance = length;
    m_yawDegrees = glm::degrees(std::atan2(offset.z, offset.x));
    m_pitchDegrees =
        std::clamp(glm::degrees(std::asin(std::clamp(offset.y / length, -1.0f, 1.0f))), kMinPitch,
                   kMaxPitch);
    return true;
}

glm::vec3 OrbitCamera::Position() const {
    return m_target + OffsetFromAngles(m_distance, m_yawDegrees, m_pitchDegrees);
}

Renderer::CameraView OrbitCamera::BuildView(float aspectRatio) const {
    const float aspect = aspectRatio > 0.01f ? aspectRatio : 1.0f;
    Renderer::CameraView cameraView;
    cameraView.view = glm::lookAt(Position(), m_target, glm::vec3(0.0f, 1.0f, 0.0f));
    cameraView.projection =
        glm::perspectiveRH_ZO(glm::radians(m_fovYDegrees), aspect, m_nearPlane, m_farPlane);
    return cameraView;
}

} // namespace DirectorDesk::Camera
