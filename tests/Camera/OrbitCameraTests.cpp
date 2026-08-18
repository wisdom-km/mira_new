// OrbitCameraTests: Implementation for the DirectorDesk Camera module.
// This file owns project behavior only; keep platform and dependency boundaries explicit.
// Contract coverage: orbit geometry, zoom behavior, and pitch safety limits.


#include "DirectorDesk/Camera/OrbitCamera.h"

#include <cmath>

#include <catch2/catch_test_macros.hpp>
#include <glm/geometric.hpp>

TEST_CASE("Orbit camera looks toward its target", "[camera]") {
    DirectorDesk::Camera::OrbitCamera camera;
    const auto view = camera.BuildView(16.0f / 9.0f);
    const float distance = glm::length(camera.Position() - camera.Target());
    REQUIRE(std::abs(distance - camera.Distance()) < 0.001f);
    REQUIRE(view.projection[0][0] != 0.0f);
}

TEST_CASE("Orbit camera zoom changes distance only", "[camera]") {
    DirectorDesk::Camera::OrbitCamera camera;
    const float before = camera.Distance();
    camera.Zoom(2.0f);
    REQUIRE(camera.Distance() < before);
    camera.Zoom(-8.0f);
    REQUIRE(camera.Distance() > before);
}

TEST_CASE("Orbit camera pitch is clamped", "[camera]") {
    DirectorDesk::Camera::OrbitCamera camera;
    camera.Rotate(0.0f, 1000.0f);
    REQUIRE(camera.PitchDegrees() <= 89.0f);
    camera.Rotate(0.0f, -2000.0f);
    REQUIRE(camera.PitchDegrees() >= -89.0f);
}
