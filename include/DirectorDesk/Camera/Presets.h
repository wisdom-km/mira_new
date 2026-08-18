// Presets: Public or internal interface for the DirectorDesk Camera module.
// This file owns project behavior only; keep platform and dependency boundaries explicit.

#pragma once

#include <glm/vec3.hpp>

#include <string>

namespace DirectorDesk::Camera {

enum class CameraPresetKind {
    Front,
    Side,
    OverShoulder,
    Top,
    CloseUp,
};

enum class LightPresetKind {
    Neutral,
    Warm,
    Cool,
};

struct SubjectFrame {
    glm::vec3 target{0.0f, 0.5f, 0.0f};
    float radius = 1.0f;
    bool hasSubject = false;
};

struct CameraPose {
    glm::vec3 target{0.0f, 0.5f, 0.0f};
    float distance = 6.0f;
    float yawDegrees = 40.0f;
    float pitchDegrees = 25.0f;
    float fovYDegrees = 50.0f;
};

struct LightState {
    glm::vec3 direction{0.35f, 0.80f, 0.45f};
    glm::vec3 color{1.0f, 0.98f, 0.94f};
};

[[nodiscard]] SubjectFrame FallbackSubject();
[[nodiscard]] SubjectFrame MakeSubject(const glm::vec3& position, const glm::vec3& scale);
[[nodiscard]] CameraPose ResolveCameraPreset(CameraPresetKind kind, const SubjectFrame& subject);
[[nodiscard]] LightState ResolveLightPreset(LightPresetKind kind);
[[nodiscard]] const char* CameraPresetId(CameraPresetKind kind);
[[nodiscard]] const char* LightPresetId(LightPresetKind kind);
[[nodiscard]] bool TryParseCameraPreset(const std::string& id, CameraPresetKind& out);
[[nodiscard]] bool TryParseLightPreset(const std::string& id, LightPresetKind& out);

} // namespace DirectorDesk::Camera
