// PresetTests: Implementation for the DirectorDesk Camera module.
// This file owns project behavior only; keep platform and dependency boundaries explicit.
// Contract coverage: preset poses and lighting selections remain deterministic.


#include "DirectorDesk/Camera/OrbitCamera.h"
#include "DirectorDesk/Camera/Presets.h"

#include <cmath>
#include <string>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <glm/geometric.hpp>

namespace {

bool FinitePose(const DirectorDesk::Camera::CameraPose& pose) {
    return std::isfinite(pose.target.x) && std::isfinite(pose.target.y) &&
           std::isfinite(pose.target.z) && std::isfinite(pose.distance) &&
           std::isfinite(pose.yawDegrees) && std::isfinite(pose.pitchDegrees) &&
           std::isfinite(pose.fovYDegrees) && pose.distance > 0.0f;
}

} // namespace

TEST_CASE("Fallback subject is finite and marked empty", "[camera][preset]") {
    const auto subject = DirectorDesk::Camera::FallbackSubject();
    REQUIRE_FALSE(subject.hasSubject);
    REQUIRE(subject.radius > 0.0f);
    REQUIRE(std::isfinite(subject.target.y));
}

TEST_CASE("MakeSubject uses the node position and scale", "[camera][preset]") {
    const auto subject =
        DirectorDesk::Camera::MakeSubject(glm::vec3(2.0f, 0.0f, -1.0f), glm::vec3(2.0f, 4.0f, 2.0f));
    REQUIRE(subject.hasSubject);
    REQUIRE(subject.target.x == Catch::Approx(2.0f));
    REQUIRE(subject.target.z == Catch::Approx(-1.0f));
    REQUIRE(subject.radius == Catch::Approx(2.0f));
}

TEST_CASE("Camera presets are deterministic and finite", "[camera][preset]") {
    const auto subject =
        DirectorDesk::Camera::MakeSubject(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f));
    const auto frontA = DirectorDesk::Camera::ResolveCameraPreset(
        DirectorDesk::Camera::CameraPresetKind::Front, subject);
    const auto frontB = DirectorDesk::Camera::ResolveCameraPreset(
        DirectorDesk::Camera::CameraPresetKind::Front, subject);
    REQUIRE(FinitePose(frontA));
    REQUIRE(frontA.distance == Catch::Approx(frontB.distance));
    REQUIRE(frontA.yawDegrees == Catch::Approx(frontB.yawDegrees));
}

TEST_CASE("Front looks from +Z and side from +X", "[camera][preset]") {
    const auto subject = DirectorDesk::Camera::FallbackSubject();
    DirectorDesk::Camera::OrbitCamera front;
    front.ApplyPose(DirectorDesk::Camera::ResolveCameraPreset(
        DirectorDesk::Camera::CameraPresetKind::Front, subject));
    DirectorDesk::Camera::OrbitCamera side;
    side.ApplyPose(DirectorDesk::Camera::ResolveCameraPreset(
        DirectorDesk::Camera::CameraPresetKind::Side, subject));
    REQUIRE(front.Position().z > front.Target().z);
    REQUIRE(side.Position().x > side.Target().x);
}

TEST_CASE("Top is steeper and close-up is nearer than front", "[camera][preset]") {
    const auto subject = DirectorDesk::Camera::FallbackSubject();
    const auto front = DirectorDesk::Camera::ResolveCameraPreset(
        DirectorDesk::Camera::CameraPresetKind::Front, subject);
    const auto top = DirectorDesk::Camera::ResolveCameraPreset(
        DirectorDesk::Camera::CameraPresetKind::Top, subject);
    const auto closeUp = DirectorDesk::Camera::ResolveCameraPreset(
        DirectorDesk::Camera::CameraPresetKind::CloseUp, subject);
    REQUIRE(top.pitchDegrees > front.pitchDegrees);
    REQUIRE(closeUp.distance < front.distance);
    REQUIRE(FinitePose(top));
    REQUIRE(FinitePose(closeUp));
}

TEST_CASE("Missing subject still yields a valid composition", "[camera][preset]") {
    const auto pose = DirectorDesk::Camera::ResolveCameraPreset(
        DirectorDesk::Camera::CameraPresetKind::OverShoulder,
        DirectorDesk::Camera::FallbackSubject());
    REQUIRE(FinitePose(pose));
    REQUIRE(pose.distance > 0.5f);
}

TEST_CASE("Light presets resolve to distinct directions", "[camera][preset]") {
    const auto neutral =
        DirectorDesk::Camera::ResolveLightPreset(DirectorDesk::Camera::LightPresetKind::Neutral);
    const auto warm =
        DirectorDesk::Camera::ResolveLightPreset(DirectorDesk::Camera::LightPresetKind::Warm);
    const auto cool =
        DirectorDesk::Camera::ResolveLightPreset(DirectorDesk::Camera::LightPresetKind::Cool);
    REQUIRE(glm::length(neutral.direction) > 0.0f);
    REQUIRE(warm.color.x > warm.color.z);
    REQUIRE(cool.color.z > cool.color.x);
}

TEST_CASE("Preset ids round-trip", "[camera][preset]") {
    DirectorDesk::Camera::CameraPresetKind cameraKind = DirectorDesk::Camera::CameraPresetKind::Front;
    REQUIRE(DirectorDesk::Camera::TryParseCameraPreset("over-shoulder", cameraKind));
    REQUIRE(cameraKind == DirectorDesk::Camera::CameraPresetKind::OverShoulder);
    REQUIRE(std::string(DirectorDesk::Camera::CameraPresetId(cameraKind)) == "over-shoulder");

    DirectorDesk::Camera::LightPresetKind lightKind = DirectorDesk::Camera::LightPresetKind::Neutral;
    REQUIRE(DirectorDesk::Camera::TryParseLightPreset("cool", lightKind));
    REQUIRE(std::string(DirectorDesk::Camera::LightPresetId(lightKind)) == "cool");
}
