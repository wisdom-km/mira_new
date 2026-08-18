#include "DirectorDesk/Camera/CameraManager.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

TEST_CASE("Camera manager starts with one selected camera", "[camera][manager]") {
    DirectorDesk::Camera::CameraManager manager;
    REQUIRE(manager.Cameras().size() == 1);
    REQUIRE(manager.Selected() != nullptr);
    REQUIRE(manager.SelectedId() == manager.Cameras().front().id);
}

TEST_CASE("Switching cameras keeps each orbit pose", "[camera][manager]") {
    DirectorDesk::Camera::CameraManager manager;
    const std::string first = manager.SelectedId();
    manager.Selected()->orbit.SetDistance(8.0f);
    manager.Add("Camera 2");
    const std::string second = manager.SelectedId();
    manager.Selected()->orbit.SetDistance(3.0f);
    REQUIRE(manager.Select(first));
    REQUIRE(manager.Selected()->orbit.Distance() == Catch::Approx(8.0f));
    REQUIRE(manager.Select(second));
    REQUIRE(manager.Selected()->orbit.Distance() == Catch::Approx(3.0f));
}

TEST_CASE("The last camera cannot be removed", "[camera][manager]") {
    DirectorDesk::Camera::CameraManager manager;
    const std::string first = manager.SelectedId();
    REQUIRE_FALSE(manager.Remove(first));
    manager.Add("Camera 2");
    REQUIRE(manager.Remove(first));
    REQUIRE(manager.Cameras().size() == 1);
    REQUIRE_FALSE(manager.Remove(manager.SelectedId()));
}

TEST_CASE("Apply preset updates only the selected camera", "[camera][manager]") {
    DirectorDesk::Camera::CameraManager manager;
    const std::string first = manager.SelectedId();
    const float original = manager.Selected()->orbit.Distance();
    manager.Add("Camera 2");
    manager.ApplyPreset(DirectorDesk::Camera::CameraPresetKind::CloseUp,
                        DirectorDesk::Camera::FallbackSubject());
    REQUIRE(manager.Selected()->orbit.Distance() < original);
    REQUIRE(manager.Select(first));
    REQUIRE(manager.Selected()->orbit.Distance() == Catch::Approx(original));
}

TEST_CASE("Rename empty camera name becomes 未命名", "[camera][manager]") {
    DirectorDesk::Camera::CameraManager manager;
    REQUIRE(manager.Rename(manager.SelectedId(), ""));
    REQUIRE(manager.Selected()->name == "未命名");
}
