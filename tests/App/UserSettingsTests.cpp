// UserSettingsTests: UIC-34 user-data preferences stay out of .ddproj.

#include "DirectorDesk/App/UserSettings.h"
#include "DirectorDesk/Platform/Paths.h"

#include <catch2/catch_test_macros.hpp>

#include <string>

TEST_CASE("UserSettings missing file loads defaults", "[app][settings]") {
    auto temp = DirectorDesk::Platform::Paths::TemporaryDirectory();
    REQUIRE(temp.IsOk());
    const std::string path = DirectorDesk::Platform::Paths::Join(
        temp.Value(), "导演台设置 DirectorDesk/missing-settings.json");
    REQUIRE_FALSE(DirectorDesk::Platform::Paths::Exists(path));
    auto loaded = DirectorDesk::App::LoadUserSettings(path);
    REQUIRE(loaded.IsOk());
    REQUIRE(loaded.Value().lockExportAspect);
    REQUIRE_FALSE(loaded.Value().leftFoldExplicit);
    REQUIRE(loaded.Value().showGroundGrid);
    REQUIRE(loaded.Value().viewportBackground == "neutral");
}

TEST_CASE("UserSettings roundtrip on a Chinese path", "[app][settings]") {
    auto temp = DirectorDesk::Platform::Paths::TemporaryDirectory();
    REQUIRE(temp.IsOk());
    const std::string dir =
        DirectorDesk::Platform::Paths::Join(temp.Value(), "导演台设置 DirectorDesk");
    const std::string path = DirectorDesk::Platform::Paths::Join(dir, "settings.json");
    DirectorDesk::App::UserSettings settings;
    settings.lockExportAspect = false;
    settings.leftFoldExplicit = true;
    settings.leftFolded = true;
    settings.showGroundGrid = false;
    settings.showGroundAxes = false;
    settings.showThirds = true;
    settings.showSafeFrame = true;
    settings.viewportBackground = "dark";
    REQUIRE(DirectorDesk::App::SaveUserSettings(path, settings).IsOk());
    auto loaded = DirectorDesk::App::LoadUserSettings(path);
    REQUIRE(loaded.IsOk());
    REQUIRE_FALSE(loaded.Value().lockExportAspect);
    REQUIRE(loaded.Value().leftFoldExplicit);
    REQUIRE(loaded.Value().leftFolded);
    REQUIRE_FALSE(loaded.Value().showGroundGrid);
    REQUIRE_FALSE(loaded.Value().showGroundAxes);
    REQUIRE(loaded.Value().showThirds);
    REQUIRE(loaded.Value().showSafeFrame);
    REQUIRE(loaded.Value().viewportBackground == "dark");
}

TEST_CASE("UserSettings rejects unknown background and relative save path", "[app][settings]") {
    auto temp = DirectorDesk::Platform::Paths::TemporaryDirectory();
    REQUIRE(temp.IsOk());
    const std::string path = DirectorDesk::Platform::Paths::Join(
        temp.Value(), "导演台设置 DirectorDesk/bad-bg.json");
    REQUIRE(DirectorDesk::Platform::Paths::CreateDirectories(
                DirectorDesk::Platform::Paths::Parent(path))
                .IsOk());
    REQUIRE(DirectorDesk::Platform::Paths::WriteTextFile(
                path, R"({"viewportBackground":"rainbow","showThirds":true})")
                .IsOk());
    auto loaded = DirectorDesk::App::LoadUserSettings(path);
    REQUIRE(loaded.IsOk());
    REQUIRE(loaded.Value().viewportBackground == "neutral");
    REQUIRE(loaded.Value().showThirds);
    auto relative = DirectorDesk::App::SaveUserSettings("settings.json", {});
    REQUIRE_FALSE(relative.IsOk());
}

TEST_CASE("UserSettingsFile is under DirectorDesk user data", "[app][settings]") {
    auto file = DirectorDesk::Platform::Paths::UserSettingsFile();
    REQUIRE(file.IsOk());
    REQUIRE(DirectorDesk::Platform::Paths::FileName(file.Value()) == "settings.json");
    REQUIRE(DirectorDesk::Platform::Paths::FileName(
                DirectorDesk::Platform::Paths::Parent(file.Value())) == "DirectorDesk");
}

TEST_CASE("UserSettings parse failure does not throw", "[app][settings]") {
    auto temp = DirectorDesk::Platform::Paths::TemporaryDirectory();
    REQUIRE(temp.IsOk());
    const std::string path = DirectorDesk::Platform::Paths::Join(
        temp.Value(), "导演台设置 DirectorDesk/broken.json");
    REQUIRE(DirectorDesk::Platform::Paths::CreateDirectories(
                DirectorDesk::Platform::Paths::Parent(path))
                .IsOk());
    REQUIRE(DirectorDesk::Platform::Paths::WriteTextFile(path, "{not-json").IsOk());
    auto loaded = DirectorDesk::App::LoadUserSettings(path);
    REQUIRE_FALSE(loaded.IsOk());
}

TEST_CASE("Cafe project file does not store UI preferences", "[app][settings]") {
    auto text = DirectorDesk::Platform::Paths::ReadTextFile(DD_EXAMPLE_CAFE_PROJECT);
    REQUIRE(text.IsOk());
    REQUIRE(text.Value().find("lockExportAspect") == std::string::npos);
    REQUIRE(text.Value().find("showGroundGrid") == std::string::npos);
    REQUIRE(text.Value().find("viewportBackground") == std::string::npos);
}

TEST_CASE("ViewportClearRgba maps background ids", "[app][settings]") {
    REQUIRE(DirectorDesk::App::ViewportClearRgba("neutral") == 0x2b2b2effu);
    REQUIRE(DirectorDesk::App::ViewportClearRgba("dark") == 0x1a1a1dffu);
    REQUIRE(DirectorDesk::App::ViewportClearRgba("unknown") == 0x2b2b2effu);
}
