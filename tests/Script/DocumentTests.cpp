// DocumentTests: Implementation for the DirectorDesk Script module.
// This file owns project behavior only; keep platform and dependency boundaries explicit.
// Contract coverage: script edits publish only valid snapshots and preserve external-file safety.


#include "DirectorDesk/Platform/Paths.h"
#include "DirectorDesk/Script/Document.h"

#include <catch2/catch_test_macros.hpp>

#include <string>

namespace {

std::string MakeCaseDir(const char* name) {
    auto temp = DirectorDesk::Platform::Paths::TemporaryDirectory();
    REQUIRE(temp.IsOk());
    const std::string dir = DirectorDesk::Platform::Paths::Join(
        temp.Value(), std::string("导演台剧本_DirectorDesk/") + name);
    auto created = DirectorDesk::Platform::Paths::CreateDirectories(dir);
    REQUIRE(created.IsOk());
    return dir;
}

void WriteText(const std::string& path, const std::string& text) {
    auto written = DirectorDesk::Platform::Paths::WriteTextFile(path, text);
    REQUIRE(written.IsOk());
}

} // namespace

TEST_CASE("Example cafe script loads into a published snapshot", "[script][document]") {
#ifndef DD_EXAMPLE_CAFE_SCRIPT
    SKIP("example cafe.md path is not defined");
#else
    const std::string example = DD_EXAMPLE_CAFE_SCRIPT;
    if (!DirectorDesk::Platform::Paths::Exists(example)) {
        SKIP("example cafe.md is not on disk");
    }
    DirectorDesk::Script::Document document;
    auto loaded = document.LoadFromPath(example);
    REQUIRE(loaded.IsOk());
    REQUIRE(document.HasPublishedSnapshot());
    REQUIRE(document.PublishedSnapshot().scenes.size() == 2);
    REQUIRE(document.PublishedSnapshot().scenes[0].shots.size() == 2);
    REQUIRE_FALSE(document.IsDirty());
#endif
}

TEST_CASE("Chinese path load edit and save keep UTF-8 text", "[script][document]") {
    const std::string dir = MakeCaseDir("cn-path");
    const std::string path = DirectorDesk::Platform::Paths::Join(dir, "咖啡馆剧本.md");
    WriteText(path, "## [scene:scene-a] 咖啡馆\n\n### [shot:shot-a] 过肩\n\n窗边。\n");

    DirectorDesk::Script::Document document;
    REQUIRE(document.LoadFromPath(path).IsOk());
    document.SetText(document.Text() + "\n补充一句。\n");
    REQUIRE(document.IsDirty());
    REQUIRE(document.Save().IsOk());
    REQUIRE_FALSE(document.IsDirty());

    auto saved = DirectorDesk::Platform::Paths::ReadTextFile(path);
    REQUIRE(saved.IsOk());
    REQUIRE(saved.Value().find("补充一句") != std::string::npos);
    REQUIRE(saved.Value().find("咖啡馆") != std::string::npos);

    DirectorDesk::Script::Document reloaded;
    REQUIRE(reloaded.LoadFromPath(path).IsOk());
    REQUIRE(reloaded.PublishedSnapshot().scenes[0].shots[0].body.find("补充一句") !=
            std::string::npos);
}

TEST_CASE("CRLF style is preserved on save", "[script][document]") {
    const std::string dir = MakeCaseDir("crlf");
    const std::string path = DirectorDesk::Platform::Paths::Join(dir, "换行.md");
    WriteText(path, "## [scene:scene-a] 场\r\n### [shot:shot-a] 镜\r\n正文\r\n");

    DirectorDesk::Script::Document document;
    REQUIRE(document.LoadFromPath(path).IsOk());
    REQUIRE(document.Save().IsOk());
    auto saved = DirectorDesk::Platform::Paths::ReadTextFile(path);
    REQUIRE(saved.IsOk());
    REQUIRE(saved.Value().find("\r\n") != std::string::npos);
}

TEST_CASE("Invalid UTF-8 load keeps the previous snapshot", "[script][document]") {
    DirectorDesk::Script::Document document;
    REQUIRE(document.LoadFromText("## [scene:scene-a] 场\n### [shot:shot-a] 镜\n正文\n").IsOk());
    REQUIRE(document.PublishedSnapshot().scenes.size() == 1);

    std::string bad = "## [scene:scene-b] 新场\n";
    bad.push_back(static_cast<char>(0xff));
    const auto loaded = document.LoadFromText(bad);
    REQUIRE_FALSE(loaded.IsOk());
    REQUIRE(document.PublishedSnapshot().scenes.size() == 1);
    REQUIRE(document.PublishedSnapshot().scenes[0].id == "scene-a");
}

TEST_CASE("External file change blocks overwrite", "[script][document]") {
    const std::string dir = MakeCaseDir("external");
    const std::string path = DirectorDesk::Platform::Paths::Join(dir, "外部.md");
    WriteText(path, "## [scene:scene-a] 场\n### [shot:shot-a] 镜\n");

    DirectorDesk::Script::Document document;
    REQUIRE(document.LoadFromPath(path).IsOk());
    document.SetText(document.Text() + "本地修改\n");
    WriteText(path, "## [scene:scene-a] 场\n### [shot:shot-a] 镜\n外部修改\n");

    const auto saved = document.Save();
    REQUIRE_FALSE(saved.IsOk());
    auto disk = DirectorDesk::Platform::Paths::ReadTextFile(path);
    REQUIRE(disk.IsOk());
    REQUIRE(disk.Value().find("外部修改") != std::string::npos);
}

TEST_CASE("Insert scene and shot write stable IDs", "[script][document]") {
    DirectorDesk::Script::Document document;
    document.InsertScene();
    document.InsertShot();
    REQUIRE(document.HasPublishedSnapshot());
    REQUIRE(document.PublishedSnapshot().scenes.size() == 1);
    REQUIRE(document.PublishedSnapshot().scenes[0].shots.size() == 1);
    REQUIRE(document.Text().find("[scene:") != std::string::npos);
    REQUIRE(document.Text().find("[shot:") != std::string::npos);
    REQUIRE(document.SelectedShotId() == document.PublishedSnapshot().scenes[0].shots[0].id);
}

TEST_CASE("Selecting a shot is reflected on the document", "[script][document]") {
    DirectorDesk::Script::Document document;
    REQUIRE(document
                .LoadFromText("## [scene:scene-a] 场\n### [shot:shot-a] 一\n### [shot:shot-b] 二\n")
                .IsOk());
    document.SelectShot("shot-b");
    REQUIRE(document.SelectedShotId() == "shot-b");
    document.SelectShot("missing");
    REQUIRE(document.SelectedShotId() == "shot-b");
}

TEST_CASE("UTF-8 BOM file loads from a Chinese path", "[script][document]") {
    const std::string dir = MakeCaseDir("bom");
    const std::string path = DirectorDesk::Platform::Paths::Join(dir, "带BOM剧本.md");
    const std::string body = "## [scene:scene-a] 场\n### [shot:shot-a] 镜\n正文\n";
    WriteText(path, std::string("\xEF\xBB\xBF") + body);

    DirectorDesk::Script::Document document;
    REQUIRE(document.LoadFromPath(path).IsOk());
    REQUIRE(document.PublishedSnapshot().scenes[0].id == "scene-a");
    REQUIRE(document.Text().find('\xEF') == std::string::npos);
}
