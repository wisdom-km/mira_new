// SkillRunnerTests: skill.json copy-output imports a local JSON.

#include "DirectorDesk/AI/SkillRunner.h"
#include "DirectorDesk/Platform/Paths.h"

#include <catch2/catch_test_macros.hpp>

#include <string>

TEST_CASE("RunSkill copy-output copies storyboard-import json", "[ai][skill]") {
    auto temp = DirectorDesk::Platform::Paths::TemporaryDirectory();
    REQUIRE(temp.IsOk());
    const std::string skillDir =
        DirectorDesk::Platform::Paths::Join(temp.Value(), "导演台Skill/storyboard");
    const std::string outDir =
        DirectorDesk::Platform::Paths::Join(temp.Value(), "导演台Skill/out");
    REQUIRE(DirectorDesk::Platform::Paths::CreateDirectories(skillDir).IsOk());
    REQUIRE(DirectorDesk::Platform::Paths::WriteTextFile(
                DirectorDesk::Platform::Paths::Join(skillDir, "skill.json"),
                R"({"format":"DirectorDeskSkillRun","formatVersion":1,"output":"storyboard-import.json","builtin":"copy-output"})")
                .IsOk());
    REQUIRE(DirectorDesk::Platform::Paths::WriteTextFile(
                DirectorDesk::Platform::Paths::Join(skillDir, "storyboard-import.json"),
                R"({"format":"DirectorDeskStoryboardImport","formatVersion":1,"title":"t","scenes":[]})")
                .IsOk());
    DirectorDesk::AI::SkillRunRequest request;
    request.skillDirectory = skillDir;
    request.outputDirectory = outDir;
    auto ran = DirectorDesk::AI::RunSkill(request);
    REQUIRE(ran.IsOk());
    REQUIRE(ran.Value().outputJsonPath.find(outDir) != std::string::npos);
    REQUIRE(DirectorDesk::Platform::Paths::Exists(ran.Value().outputJsonPath));
}

TEST_CASE("RunSkill without skill.json asks the user to use an Agent", "[ai][skill]") {
    auto temp = DirectorDesk::Platform::Paths::TemporaryDirectory();
    REQUIRE(temp.IsOk());
    const std::string skillDir =
        DirectorDesk::Platform::Paths::Join(temp.Value(), "导演台Skill/plain");
    REQUIRE(DirectorDesk::Platform::Paths::CreateDirectories(skillDir).IsOk());
    DirectorDesk::AI::SkillRunRequest request;
    request.skillDirectory = skillDir;
    auto ran = DirectorDesk::AI::RunSkill(request);
    REQUIRE_FALSE(ran.IsOk());
    REQUIRE(ran.GetError().userMessage.find("Agent") != std::string::npos);
}

TEST_CASE("RunSkill openai-compat-json mock writes storyboard json from script", "[ai][skill]") {
    auto temp = DirectorDesk::Platform::Paths::TemporaryDirectory();
    REQUIRE(temp.IsOk());
    const std::string skillDir =
        DirectorDesk::Platform::Paths::Join(temp.Value(), "导演台Skill/llm");
    const std::string outDir =
        DirectorDesk::Platform::Paths::Join(temp.Value(), "导演台Skill/llm-out");
    REQUIRE(DirectorDesk::Platform::Paths::CreateDirectories(skillDir).IsOk());
    REQUIRE(DirectorDesk::Platform::Paths::WriteTextFile(
                DirectorDesk::Platform::Paths::Join(skillDir, "skill.json"),
                R"({"format":"DirectorDeskSkillRun","formatVersion":1,"output":"storyboard-import.json","builtin":"openai-compat-json"})")
                .IsOk());
    DirectorDesk::AI::SkillRunRequest request;
    request.skillDirectory = skillDir;
    request.outputDirectory = outDir;
    request.provider = "mock";
    request.scriptText = "## [scene:s1] 咖啡馆\n\n### [shot:a] 过肩\n";
    request.projectName = "测试片";
    auto ran = DirectorDesk::AI::RunSkill(request);
    REQUIRE(ran.IsOk());
    REQUIRE_FALSE(ran.Value().usedNetwork);
    auto text = DirectorDesk::Platform::Paths::ReadTextFile(ran.Value().outputJsonPath);
    REQUIRE(text.IsOk());
    REQUIRE(text.Value().find("DirectorDeskStoryboardImport") != std::string::npos);
    REQUIRE(text.Value().find("测试片") != std::string::npos);
}
