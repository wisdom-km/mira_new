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
    REQUIRE(loaded.Value().uiScale == 0.0f);
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
    settings.aiProvider = "mock";
    settings.aiApiKey = "sk-测试";
    settings.aiBaseUrl = "https://api.example.com";
    settings.aiChatModel = "deepseek-chat";
    settings.uiScale = 1.5f;
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
    REQUIRE(loaded.Value().aiProvider == "mock");
    REQUIRE(loaded.Value().aiApiKey == "sk-测试");
    REQUIRE(loaded.Value().aiBaseUrl == "https://api.example.com");
    REQUIRE(loaded.Value().aiChatModel == "deepseek-chat");
    REQUIRE(loaded.Value().uiScale == 1.5f);
    REQUIRE_FALSE(loaded.Value().openLastProject);
    REQUIRE(loaded.Value().lastProjectPath.empty());
    REQUIRE(loaded.Value().defaultExportDirectory.empty());
    REQUIRE(loaded.Value().exportResolutionId == "1080p");
    REQUIRE(loaded.Value().exportTransparent);
}

TEST_CASE("UserSettings roundtrip last project and export defaults", "[app][settings]") {
    auto temp = DirectorDesk::Platform::Paths::TemporaryDirectory();
    REQUIRE(temp.IsOk());
    const std::string dir =
        DirectorDesk::Platform::Paths::Join(temp.Value(), "导演台设置 DirectorDesk");
    const std::string path = DirectorDesk::Platform::Paths::Join(dir, "settings-export.json");
    DirectorDesk::App::UserSettings settings;
    settings.openLastProject = true;
    settings.lastProjectPath = "D:/工程/咖啡馆.ddproj";
    settings.defaultExportDirectory = "D:/导出";
    settings.exportResolutionId = "2k";
    settings.exportTransparent = false;
    REQUIRE(DirectorDesk::App::SaveUserSettings(path, settings).IsOk());
    auto loaded = DirectorDesk::App::LoadUserSettings(path);
    REQUIRE(loaded.IsOk());
    REQUIRE(loaded.Value().openLastProject);
    REQUIRE(loaded.Value().lastProjectPath == "D:/工程/咖啡馆.ddproj");
    REQUIRE(loaded.Value().defaultExportDirectory == "D:/导出");
    REQUIRE(loaded.Value().exportResolutionId == "2k");
    REQUIRE_FALSE(loaded.Value().exportTransparent);
}

TEST_CASE("UserSettings roundtrip defaultSkillId", "[app][settings][uic59]") {
    auto temp = DirectorDesk::Platform::Paths::TemporaryDirectory();
    REQUIRE(temp.IsOk());
    const std::string dir =
        DirectorDesk::Platform::Paths::Join(temp.Value(), "导演台设置 DirectorDesk");
    const std::string path = DirectorDesk::Platform::Paths::Join(dir, "settings-skill.json");
    DirectorDesk::App::UserSettings settings;
    settings.defaultSkillId = "skill-storyboard";
    REQUIRE(DirectorDesk::App::SaveUserSettings(path, settings).IsOk());
    auto loaded = DirectorDesk::App::LoadUserSettings(path);
    REQUIRE(loaded.IsOk());
    REQUIRE(loaded.Value().defaultSkillId == "skill-storyboard");
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
    REQUIRE(text.Value().find("uiScale") == std::string::npos);
    REQUIRE(text.Value().find("openLastProject") == std::string::npos);
    REQUIRE(text.Value().find("lastProjectPath") == std::string::npos);
    REQUIRE(text.Value().find("defaultExportDirectory") == std::string::npos);
    REQUIRE(text.Value().find("aiApiKey") == std::string::npos);
}

TEST_CASE("ViewportClearRgba maps background ids", "[app][settings]") {
    REQUIRE(DirectorDesk::App::ViewportClearRgba("neutral") == 0x2b2b2effu);
    REQUIRE(DirectorDesk::App::ViewportClearRgba("dark") == 0x1a1a1dffu);
    REQUIRE(DirectorDesk::App::ViewportClearRgba("unknown") == 0x2b2b2effu);
}

TEST_CASE("SanitizeUiScale snaps to allowed steps", "[app][settings]") {
    REQUIRE(DirectorDesk::App::SanitizeUiScale(0.0f) == 0.0f);
    REQUIRE(DirectorDesk::App::SanitizeUiScale(-1.0f) == 0.0f);
    REQUIRE(DirectorDesk::App::SanitizeUiScale(1.0f) == 1.0f);
    REQUIRE(DirectorDesk::App::SanitizeUiScale(1.25f) == 1.25f);
    REQUIRE(DirectorDesk::App::SanitizeUiScale(1.2f) == 1.25f);
    REQUIRE(DirectorDesk::App::SanitizeUiScale(3.0f) == 2.0f);
}

TEST_CASE("ResolveUiScale uses stored value or first-launch default", "[app][settings]") {
    REQUIRE(DirectorDesk::App::ResolveUiScale(1.5f, 1280, 1.0f) == 1.5f);
    REQUIRE(DirectorDesk::App::ResolveUiScale(0.0f, 1280, 1.0f) == 1.0f);
    REQUIRE(DirectorDesk::App::ResolveUiScale(0.0f, 2560, 1.0f) == 1.25f);
    REQUIRE(DirectorDesk::App::ResolveUiScale(0.0f, 2560, 1.5f) == 1.0f);
}

TEST_CASE("UserSettings missing uiScale key stays unset", "[app][settings]") {
    auto temp = DirectorDesk::Platform::Paths::TemporaryDirectory();
    REQUIRE(temp.IsOk());
    const std::string path = DirectorDesk::Platform::Paths::Join(
        temp.Value(), "导演台设置 DirectorDesk/legacy-settings.json");
    REQUIRE(DirectorDesk::Platform::Paths::CreateDirectories(
                DirectorDesk::Platform::Paths::Parent(path))
                .IsOk());
    REQUIRE(DirectorDesk::Platform::Paths::WriteTextFile(path, R"({"showThirds":true})").IsOk());
    auto loaded = DirectorDesk::App::LoadUserSettings(path);
    REQUIRE(loaded.IsOk());
    REQUIRE(loaded.Value().showThirds);
    REQUIRE(loaded.Value().uiScale == 0.0f);
}
