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
