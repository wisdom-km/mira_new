// LibraryTests: Implementation for the DirectorDesk Asset module.
// This file owns project behavior only; keep platform and dependency boundaries explicit.
// Contract coverage: local asset indexing, filtering, stable IDs, and missing-source handling.


#include "DirectorDesk/Asset/Library.h"
#include "DirectorDesk/Platform/Paths.h"

#include <catch2/catch_test_macros.hpp>

#include <cstdint>
#include <filesystem>
#include <string>

namespace {

std::string MakeCaseDir(const char* name) {
    auto temp = DirectorDesk::Platform::Paths::TemporaryDirectory();
    REQUIRE(temp.IsOk());
    const std::string dir = DirectorDesk::Platform::Paths::Join(
        temp.Value(), std::string("导演台资源库_DirectorDesk/") + name);
    REQUIRE(DirectorDesk::Platform::Paths::CreateDirectories(dir).IsOk());
    return dir;
}

std::string WriteTinyObj(const std::string& directory, const std::string& fileName) {
    const std::string path = DirectorDesk::Platform::Paths::Join(directory, fileName);
    const std::string text = "v 0 0 0\nv 1 0 0\nv 0 1 0\nf 1 2 3\n";
    REQUIRE(DirectorDesk::Platform::Paths::WriteTextFile(path, text).IsOk());
    return path;
}

} // namespace

TEST_CASE("Library import persists across reopen", "[asset][library]") {
    const std::string root = MakeCaseDir("persist");
    const std::string models = DirectorDesk::Platform::Paths::Join(root, "models");
    const std::string libraryDir = DirectorDesk::Platform::Paths::Join(root, "library");
    const std::string model = WriteTinyObj(models, "椅子.obj");

    DirectorDesk::Asset::Library library;
    REQUIRE(library.Open(libraryDir).IsOk());
    auto imported = library.Import(model, DirectorDesk::Asset::AssetOrigin::User);
    REQUIRE(imported.IsOk());
    const std::string id = imported.Value().id;
    REQUIRE(imported.Value().format == "obj");
    REQUIRE(imported.Value().origin == DirectorDesk::Asset::AssetOrigin::User);

    DirectorDesk::Asset::Library reopened;
    REQUIRE(reopened.Open(libraryDir).IsOk());
    REQUIRE(reopened.Find(id) != nullptr);
    REQUIRE(reopened.Find(id)->name.find("椅子") != std::string::npos);
}

TEST_CASE("Duplicate import of the same path keeps one id", "[asset][library]") {
    const std::string root = MakeCaseDir("dup");
    const std::string model = WriteTinyObj(root, "cube.obj");
    const std::string libraryDir = DirectorDesk::Platform::Paths::Join(root, "library");
    DirectorDesk::Asset::Library library;
    REQUIRE(library.Open(libraryDir).IsOk());
    auto first = library.Import(model, DirectorDesk::Asset::AssetOrigin::User);
    auto second = library.Import(model, DirectorDesk::Asset::AssetOrigin::User);
    REQUIRE(first.IsOk());
    REQUIRE(second.IsOk());
    REQUIRE(first.Value().id == second.Value().id);
    REQUIRE(library.Assets().size() == 1);
}

TEST_CASE("Missing source is marked without dropping the index", "[asset][library]") {
    const std::string root = MakeCaseDir("missing");
    const std::string model = WriteTinyObj(root, "gone.obj");
    const std::string libraryDir = DirectorDesk::Platform::Paths::Join(root, "library");
    DirectorDesk::Asset::Library library;
    REQUIRE(library.Open(libraryDir).IsOk());
    auto imported = library.Import(model, DirectorDesk::Asset::AssetOrigin::User);
    REQUIRE(imported.IsOk());
    REQUIRE(DirectorDesk::Platform::Paths::WriteTextFile(model + ".bak", "x").IsOk());
    std::filesystem::remove(std::filesystem::u8path(model));

    library.Refresh();
    REQUIRE(library.Assets().size() == 1);
    REQUIRE_FALSE(library.Find(imported.Value().id)->sourceExists);
    auto found = library.Query("gone", "all");
    REQUIRE(found.size() == 1);
}

TEST_CASE("Remove drops a missing asset from the index", "[asset][library]") {
    const std::string root = MakeCaseDir("remove-missing");
    const std::string model = WriteTinyObj(root, "gone.obj");
    const std::string libraryDir = DirectorDesk::Platform::Paths::Join(root, "library");
    DirectorDesk::Asset::Library library;
    REQUIRE(library.Open(libraryDir).IsOk());
    auto imported = library.Import(model, DirectorDesk::Asset::AssetOrigin::User);
    REQUIRE(imported.IsOk());
    std::filesystem::remove(std::filesystem::u8path(model));
    library.Refresh();
    REQUIRE_FALSE(library.Find(imported.Value().id)->sourceExists);
    REQUIRE(library.Remove(imported.Value().id));
    REQUIRE(library.Find(imported.Value().id) == nullptr);

    DirectorDesk::Asset::Library reopened;
    REQUIRE(reopened.Open(libraryDir).IsOk());
    REQUIRE(reopened.Find(imported.Value().id) == nullptr);
}

TEST_CASE("Corrupt index recovers and stays usable", "[asset][library]") {
    const std::string root = MakeCaseDir("corrupt");
    const std::string libraryDir = DirectorDesk::Platform::Paths::Join(root, "library");
    REQUIRE(DirectorDesk::Platform::Paths::CreateDirectories(libraryDir).IsOk());
    const std::string index = DirectorDesk::Platform::Paths::Join(libraryDir, "index.json");
    REQUIRE(DirectorDesk::Platform::Paths::WriteTextFile(index, "{not-json").IsOk());

    DirectorDesk::Asset::Library library;
    REQUIRE(library.Open(libraryDir).IsOk());
    REQUIRE(library.RecoveredFromCorruptIndex());
    REQUIRE(library.Assets().empty());
    const std::string model = WriteTinyObj(root, "ok.obj");
    REQUIRE(library.Import(model, DirectorDesk::Asset::AssetOrigin::Builtin).IsOk());
    REQUIRE(library.Assets().size() == 1);
}

TEST_CASE("Search and origin filters", "[asset][library]") {
    const std::string root = MakeCaseDir("query");
    const std::string libraryDir = DirectorDesk::Platform::Paths::Join(root, "library");
    DirectorDesk::Asset::Library library;
    REQUIRE(library.Open(libraryDir).IsOk());
    REQUIRE(library.Import(WriteTinyObj(root, "hero.obj"), DirectorDesk::Asset::AssetOrigin::Builtin)
                .IsOk());
    REQUIRE(library.Import(WriteTinyObj(root, "prop.obj"), DirectorDesk::Asset::AssetOrigin::User)
                .IsOk());
    REQUIRE(library.Query("hero", "all").size() == 1);
    REQUIRE(library.Query("", "builtin").size() == 1);
    REQUIRE(library.Query("", "online").empty());
}

TEST_CASE("Official origin and version persist in the library index", "[asset][library]") {
    const std::string root = MakeCaseDir("official-index");
    const std::string libraryDir = DirectorDesk::Platform::Paths::Join(root, "library");
    const std::string model = WriteTinyObj(root, "chair.obj");
    DirectorDesk::Asset::Library library;
    REQUIRE(library.Open(libraryDir).IsOk());
    DirectorDesk::Asset::LibraryAsset item;
    item.id = "basic-chair";
    item.name = "椅子";
    item.sourcePath = model;
    item.format = "obj";
    item.origin = DirectorDesk::Asset::AssetOrigin::OnlineCache;
    item.version = "1.0.0";
    item.entrypoint = "model/chair.obj";
    REQUIRE(library.Upsert(item).IsOk());

    auto index = DirectorDesk::Platform::Paths::ReadTextFile(
        DirectorDesk::Platform::Paths::Join(libraryDir, "index.json"));
    REQUIRE(index.IsOk());
    REQUIRE(index.Value().find("\"origin\": \"official\"") != std::string::npos);
    REQUIRE(index.Value().find("\"version\": \"1.0.0\"") != std::string::npos);

    DirectorDesk::Asset::Library reopened;
    REQUIRE(reopened.Open(libraryDir).IsOk());
    REQUIRE(reopened.Find("basic-chair") != nullptr);
    REQUIRE(reopened.Find("basic-chair")->origin == DirectorDesk::Asset::AssetOrigin::OnlineCache);
    REQUIRE(reopened.Find("basic-chair")->version == "1.0.0");
    REQUIRE(reopened.Find("basic-chair")->entrypoint == "model/chair.obj");
    REQUIRE(reopened.Query("", "online").size() == 1);
    REQUIRE(reopened.Query("", "official").size() == 1);
}

TEST_CASE("Legacy user and online origin strings still load", "[asset][library]") {
    const std::string root = MakeCaseDir("legacy-origin");
    const std::string libraryDir = DirectorDesk::Platform::Paths::Join(root, "library");
    REQUIRE(DirectorDesk::Platform::Paths::CreateDirectories(libraryDir).IsOk());
    const std::string model = WriteTinyObj(root, "legacy.obj");
    const std::string json =
        std::string("{\n  \"schemaVersion\": 1,\n  \"assets\": [\n    {\n") +
        "      \"id\": \"legacy-1\",\n      \"name\": \"旧\",\n      \"sourcePath\": \"" +
        DirectorDesk::Platform::Paths::NormalizeSlashes(model) +
        "\",\n      \"format\": \"obj\",\n      \"origin\": \"online\"\n    }\n  ]\n}\n";
    REQUIRE(DirectorDesk::Platform::Paths::WriteTextFile(
                DirectorDesk::Platform::Paths::Join(libraryDir, "index.json"), json)
                .IsOk());
    DirectorDesk::Asset::Library library;
    REQUIRE(library.Open(libraryDir).IsOk());
    REQUIRE(library.Find("legacy-1") != nullptr);
    REQUIRE(library.Find("legacy-1")->origin == DirectorDesk::Asset::AssetOrigin::OnlineCache);
}

TEST_CASE("Library ids are stable for the same path", "[asset][library]") {
    const std::string root = MakeCaseDir("id");
    const std::string model = WriteTinyObj(root, "same.obj");
    const std::string a = DirectorDesk::Asset::Library::MakeId(model);
    const std::string b = DirectorDesk::Asset::Library::MakeId(model);
    REQUIRE(a == b);
    REQUIRE(a.find("local-") == 0);
}

TEST_CASE("Library import accepts SKILL.md", "[asset][library][skill]") {
    const std::string root = MakeCaseDir("skill-copy");
    const std::string libraryDir = DirectorDesk::Platform::Paths::Join(root, "library");
    const std::string skillPath = DirectorDesk::Platform::Paths::Join(root, "SKILL.md");
    REQUIRE(DirectorDesk::Platform::Paths::WriteTextFile(skillPath, "# Skill\n在 Agent 里跑。\n")
                .IsOk());
    REQUIRE(DirectorDesk::Platform::Paths::WriteTextFile(
                DirectorDesk::Platform::Paths::Join(root, "skill.json"),
                R"({"format":"DirectorDeskSkillRun","formatVersion":1,"output":"storyboard-import.json"})")
                .IsOk());
    REQUIRE(DirectorDesk::Platform::Paths::WriteTextFile(
                DirectorDesk::Platform::Paths::Join(root, "prompt.md"), "只输出 JSON。\n")
                .IsOk());
    DirectorDesk::Asset::Library library;
    REQUIRE(library.Open(libraryDir).IsOk());
    auto imported = library.Import(skillPath, DirectorDesk::Asset::AssetOrigin::User);
    REQUIRE(imported.IsOk());
    REQUIRE(imported.Value().format == "skill");
    REQUIRE(imported.Value().category == "skill");
    REQUIRE(imported.Value().entrypoint == "SKILL.md");
    REQUIRE(imported.Value().sourcePath != skillPath);
    REQUIRE(DirectorDesk::Platform::Paths::FileName(imported.Value().sourcePath) == "SKILL.md");
    REQUIRE(DirectorDesk::Platform::Paths::Exists(imported.Value().sourcePath));
    auto copied = DirectorDesk::Platform::Paths::ReadTextFile(imported.Value().sourcePath);
    REQUIRE(copied.IsOk());
    REQUIRE(copied.Value().find("# Skill") != std::string::npos);
    const std::string copiedJson = DirectorDesk::Platform::Paths::Join(
        DirectorDesk::Platform::Paths::Parent(imported.Value().sourcePath), "skill.json");
    REQUIRE(DirectorDesk::Platform::Paths::Exists(copiedJson));
    const std::string copiedPrompt = DirectorDesk::Platform::Paths::Join(
        DirectorDesk::Platform::Paths::Parent(imported.Value().sourcePath), "prompt.md");
    REQUIRE(DirectorDesk::Platform::Paths::Exists(copiedPrompt));
    REQUIRE(library.Find(imported.Value().id) != nullptr);
}
