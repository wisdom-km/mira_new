#include "DirectorDesk/Camera/Presets.h"

#include <algorithm>
#include <cmath>

namespace DirectorDesk::Camera {
namespace {

constexpr float kMinRadius = 0.35f;
constexpr float kMaxRadius = 20.0f;

float ClampRadius(float radius) {
    return std::clamp(radius, kMinRadius, kMaxRadius);
}

} // namespace

SubjectFrame FallbackSubject() {
    SubjectFrame frame;
    frame.target = glm::vec3(0.0f, 0.5f, 0.0f);
    frame.radius = 1.0f;
    frame.hasSubject = false;
    return frame;
}

SubjectFrame MakeSubject(const glm::vec3& position, const glm::vec3& scale) {
    SubjectFrame frame;
    const float height = std::max(std::abs(scale.y), 0.5f);
    frame.target = position + glm::vec3(0.0f, height * 0.5f, 0.0f);
    frame.radius = ClampRadius(
        std::max({std::abs(scale.x), std::abs(scale.y), std::abs(scale.z)}) * 0.5f);
    frame.hasSubject = true;
    return frame;
}

CameraPose ResolveCameraPreset(CameraPresetKind kind, const SubjectFrame& subject) {
    CameraPose pose;
    pose.target = subject.target;
    const float radius = ClampRadius(subject.radius);

    switch (kind) {
    case CameraPresetKind::Front:
        pose.yawDegrees = 90.0f;
        pose.pitchDegrees = 8.0f;
        pose.distance = radius * 4.0f;
        pose.fovYDegrees = 45.0f;
        break;
    case CameraPresetKind::Side:
        pose.yawDegrees = 0.0f;
        pose.pitchDegrees = 8.0f;
        pose.distance = radius * 4.0f;
        pose.fovYDegrees = 45.0f;
        break;
    case CameraPresetKind::OverShoulder:
        pose.yawDegrees = 60.0f;
        pose.pitchDegrees = 10.0f;
        pose.distance = radius * 2.8f;
        pose.fovYDegrees = 40.0f;
        pose.target += glm::vec3(-radius * 0.25f, radius * 0.20f, 0.0f);
        break;
    case CameraPresetKind::Top:
        pose.yawDegrees = 90.0f;
        pose.pitchDegrees = 75.0f;
        pose.distance = radius * 6.0f;
        pose.fovYDegrees = 50.0f;
        break;
    case CameraPresetKind::CloseUp:
        pose.yawDegrees = 90.0f;
        pose.pitchDegrees = 12.0f;
        pose.distance = radius * 1.6f;
        pose.fovYDegrees = 35.0f;
        pose.target += glm::vec3(0.0f, radius * 0.35f, 0.0f);
        break;
    }
    return pose;
}

LightState ResolveLightPreset(LightPresetKind kind) {
    LightState light;
    switch (kind) {
    case LightPresetKind::Neutral:
        light.direction = glm::vec3(0.35f, 0.80f, 0.45f);
        light.color = glm::vec3(1.00f, 0.98f, 0.94f);
        break;
    case LightPresetKind::Warm:
        light.direction = glm::vec3(0.25f, 0.75f, 0.55f);
        light.color = glm::vec3(1.00f, 0.88f, 0.72f);
        break;
    case LightPresetKind::Cool:
        light.direction = glm::vec3(0.45f, 0.70f, 0.30f);
        light.color = glm::vec3(0.82f, 0.90f, 1.00f);
        break;
    }
    return light;
}

const char* CameraPresetId(CameraPresetKind kind) {
    switch (kind) {
    case CameraPresetKind::Front:
        return "front";
    case CameraPresetKind::Side:
        return "side";
    case CameraPresetKind::OverShoulder:
        return "over-shoulder";
    case CameraPresetKind::Top:
        return "top";
    case CameraPresetKind::CloseUp:
        return "close-up";
    }
    return "front";
}

const char* LightPresetId(LightPresetKind kind) {
    switch (kind) {
    case LightPresetKind::Neutral:
        return "neutral";
    case LightPresetKind::Warm:
        return "warm";
    case LightPresetKind::Cool:
        return "cool";
    }
    return "neutral";
}

bool TryParseCameraPreset(const std::string& id, CameraPresetKind& out) {
    if (id == "front") {
        out = CameraPresetKind::Front;
        return true;
    }
    if (id == "side") {
        out = CameraPresetKind::Side;
        return true;
    }
    if (id == "over-shoulder") {
        out = CameraPresetKind::OverShoulder;
        return true;
    }
    if (id == "top") {
        out = CameraPresetKind::Top;
        return true;
    }
    if (id == "close-up") {
        out = CameraPresetKind::CloseUp;
        return true;
    }
    return false;
}

bool TryParseLightPreset(const std::string& id, LightPresetKind& out) {
    if (id == "neutral") {
        out = LightPresetKind::Neutral;
        return true;
    }
    if (id == "warm") {
        out = LightPresetKind::Warm;
        return true;
    }
    if (id == "cool") {
        out = LightPresetKind::Cool;
        return true;
    }
    return false;
}

} // namespace DirectorDesk::Camera
