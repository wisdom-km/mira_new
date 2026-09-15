// ImportStoryboardTests: storyboard-import 1 (FND-30).
#include "DirectorDesk/Script/ImportStoryboard.h"
#include "DirectorDesk/Script/Parser.h"

#include <catch2/catch_test_macros.hpp>

namespace {

const char* kFullJson = R"({
  "format": "DirectorDeskStoryboardImport",
  "formatVersion": 1,
  "source": { "tool": "test", "version": "1.0.0" },
  "title": "咖啡馆短片",
  "scenes": [
    {
      "id": "scene-cafe-day",
      "title": "咖啡馆 - 日 - 内",
      "body": "场次说明。",
      "shots": [
        {
          "id": "shot-cafe-001",
          "title": "过肩",
          "body": "A 坐在窗边，看向街道。",
          "meta": { "景别": "中景", "运镜": "推", "时长": "3s" }
        }
      ]
    }
  ]
})";

} // namespace

TEST_CASE("Import storyboard round-trips into script-format 1.1", "[script][import]") {
    auto imported = DirectorDesk::Script::ImportStoryboard(
        kFullJson, DirectorDesk::Script::StoryboardImportMode::Replace, nullptr);
    REQUIRE(imported.IsOk());
    REQUIRE(imported.Value().sceneCount == 1);
    REQUIRE(imported.Value().shotCount == 1);
    const auto parsed = DirectorDesk::Script::Parser::Parse(imported.Value().markdown);
    REQUIRE(parsed.completed);
    REQUIRE(parsed.snapshot.documentTitle == "咖啡馆短片");
    REQUIRE(parsed.snapshot.scenes[0].id == "scene-cafe-day");
    REQUIRE(parsed.snapshot.scenes[0].shots[0].id == "shot-cafe-001");
    REQUIRE(parsed.snapshot.scenes[0].shots[0].meta.size() == 3);
    REQUIRE(parsed.snapshot.scenes[0].shots[0].meta[0].key == "景别");
    REQUIRE(parsed.snapshot.scenes[0].shots[0].meta[0].value == "中景");
    REQUIRE(parsed.snapshot.scenes[0].shots[0].body.find("窗边") != std::string::npos);
}

TEST_CASE("Import storyboard generates IDs when missing", "[script][import]") {
    const char* json =
        R"({"format":"DirectorDeskStoryboardImport","formatVersion":1,"title":"T",
            "scenes":[{"title":"场","shots":[{"title":"镜","body":"b"}]}]})";
    auto imported = DirectorDesk::Script::ImportStoryboard(
        json, DirectorDesk::Script::StoryboardImportMode::Replace, nullptr);
    REQUIRE(imported.IsOk());
    const auto parsed = DirectorDesk::Script::Parser::Parse(imported.Value().markdown);
    REQUIRE(parsed.snapshot.scenes[0].id.rfind("scene-", 0) == 0);
    REQUIRE(parsed.snapshot.scenes[0].shots[0].id.rfind("shot-", 0) == 0);
    REQUIRE_FALSE(imported.Value().diagnostics.empty());
}

TEST_CASE("Import storyboard replaces illegal IDs", "[script][import]") {
    const char* json =
        R"({"format":"DirectorDeskStoryboardImport","formatVersion":1,"title":"T",
            "scenes":[{"id":"BAD ID","title":"场","shots":[{"id":"Nope","title":"镜","body":"b"}]}]})";
    auto imported = DirectorDesk::Script::ImportStoryboard(
        json, DirectorDesk::Script::StoryboardImportMode::Replace, nullptr);
    REQUIRE(imported.IsOk());
    const auto parsed = DirectorDesk::Script::Parser::Parse(imported.Value().markdown);
    REQUIRE(DirectorDesk::Script::Parser::Parse(imported.Value().markdown).completed);
    REQUIRE(parsed.snapshot.scenes[0].id != "BAD ID");
    REQUIRE(parsed.snapshot.scenes[0].shots[0].id != "Nope");
}

TEST_CASE("Import storyboard append remaps colliding IDs", "[script][import]") {
    auto existingParsed = DirectorDesk::Script::Parser::Parse(
        "## [scene:scene-cafe-day] 场\n### [shot:shot-cafe-001] 旧\n正文\n");
    auto imported = DirectorDesk::Script::ImportStoryboard(
        kFullJson, DirectorDesk::Script::StoryboardImportMode::Append, &existingParsed.snapshot);
    REQUIRE(imported.IsOk());
    REQUIRE(imported.Value().markdown.find("## [scene:scene-cafe-day]") == std::string::npos);
    REQUIRE(imported.Value().markdown.find("### [shot:shot-cafe-001]") == std::string::npos);
}

TEST_CASE("Import storyboard keeps Chinese meta keys", "[script][import]") {
    const char* json =
        R"({"format":"DirectorDeskStoryboardImport","formatVersion":1,"title":"T",
            "scenes":[{"id":"scene-a","title":"场","shots":[{"id":"shot-a","title":"镜","body":"b",
            "meta":{"提示词":"暖光","负面提示词":"低清"}}]}]})";
    auto imported = DirectorDesk::Script::ImportStoryboard(
        json, DirectorDesk::Script::StoryboardImportMode::Replace, nullptr);
    REQUIRE(imported.IsOk());
    const auto parsed = DirectorDesk::Script::Parser::Parse(imported.Value().markdown);
    REQUIRE(parsed.snapshot.scenes[0].shots[0].meta[0].key == "提示词");
    REQUIRE(parsed.snapshot.scenes[0].shots[0].meta[1].key == "负面提示词");
}

TEST_CASE("Import storyboard accepts empty scenes", "[script][import]") {
    const char* json =
        R"({"format":"DirectorDeskStoryboardImport","formatVersion":1,"title":"空","scenes":[]})";
    auto imported = DirectorDesk::Script::ImportStoryboard(
        json, DirectorDesk::Script::StoryboardImportMode::Replace, nullptr);
    REQUIRE(imported.IsOk());
    REQUIRE(imported.Value().sceneCount == 0);
    REQUIRE(imported.Value().markdown.find("# 空") != std::string::npos);
}

TEST_CASE("Import storyboard rejects formatVersion 2", "[script][import]") {
    const char* json =
        R"({"format":"DirectorDeskStoryboardImport","formatVersion":2,"title":"T","scenes":[]})";
    auto imported = DirectorDesk::Script::ImportStoryboard(
        json, DirectorDesk::Script::StoryboardImportMode::Replace, nullptr);
    REQUIRE_FALSE(imported.IsOk());
}
