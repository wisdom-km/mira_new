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

TEST_CASE("Library ids are stable for the same path", "[asset][library]") {
    const std::string root = MakeCaseDir("id");
    const std::string model = WriteTinyObj(root, "same.obj");
    const std::string a = DirectorDesk::Asset::Library::MakeId(model);
    const std::string b = DirectorDesk::Asset::Library::MakeId(model);
    REQUIRE(a == b);
    REQUIRE(a.find("local-") == 0);
}
