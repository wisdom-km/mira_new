#include "DirectorDesk/Asset/LoaderRegistry.h"
#include "DirectorDesk/Platform/Paths.h"

#include "TestModels.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

namespace {

std::string MakeCaseDir(const char* name) {
    auto temp = DirectorDesk::Platform::Paths::TemporaryDirectory();
    REQUIRE(temp.IsOk());
    const std::string dir = DirectorDesk::Platform::Paths::Join(
        temp.Value(), std::string("导演台模型_DirectorDesk/") + name);
    auto created = DirectorDesk::Platform::Paths::CreateDirectories(dir);
    REQUIRE(created.IsOk());
    return dir;
}

} // namespace

TEST_CASE("Default registry selects glb and obj loaders", "[asset][loader]") {
    const auto registry = DirectorDesk::Asset::CreateDefaultRegistry();
    REQUIRE(registry.FindByExtension(".glb") != nullptr);
    REQUIRE(registry.FindByExtension(".obj") != nullptr);
    REQUIRE(registry.FindByExtension(".gltf") == nullptr);
}

TEST_CASE("OBJ loads from a Chinese path and generates missing normals", "[asset][obj]") {
    const std::string dir = MakeCaseDir("obj-cn");
    const std::string path = DirectorDesk::Tests::WriteCubeObj(dir, false);
    const auto registry = DirectorDesk::Asset::CreateDefaultRegistry();
    auto loaded = registry.Load(path);
    REQUIRE(loaded.IsOk());
    REQUIRE_FALSE(loaded.Value().primitives.empty());
    REQUIRE(loaded.Value().primitives.front().vertices.size() >= 3);
    const auto normal = loaded.Value().primitives.front().vertices.front().normal;
    REQUIRE(normal.z == Catch::Approx(1.0f).margin(0.05f));
}

TEST_CASE("OBJ with normals loads triangles", "[asset][obj]") {
    const std::string dir = MakeCaseDir("obj-normals");
    const std::string path = DirectorDesk::Tests::WriteCubeObj(dir, true);
    const auto registry = DirectorDesk::Asset::CreateDefaultRegistry();
    auto loaded = registry.Load(path);
    REQUIRE(loaded.IsOk());
    REQUIRE(loaded.Value().primitives.front().indices.size() == 3);
}

TEST_CASE("Corrupt OBJ returns a parse error", "[asset][obj]") {
    const std::string dir = MakeCaseDir("obj-bad");
    const std::string path = DirectorDesk::Platform::Paths::Join(dir, "坏掉.obj");
    DirectorDesk::Tests::WriteTextFile(path, "this is not an obj file");
    const auto registry = DirectorDesk::Asset::CreateDefaultRegistry();
    auto loaded = registry.Load(path);
    REQUIRE_FALSE(loaded.IsOk());
    REQUIRE(loaded.GetError().code == DirectorDesk::Core::ErrorCode::ParseFailure);
}

TEST_CASE("Unsupported extension does not crash", "[asset][loader]") {
    const std::string dir = MakeCaseDir("unsupported");
    const std::string path = DirectorDesk::Platform::Paths::Join(dir, "model.gltf");
    DirectorDesk::Tests::WriteTextFile(path, "{}");
    const auto registry = DirectorDesk::Asset::CreateDefaultRegistry();
    auto loaded = registry.Load(path);
    REQUIRE_FALSE(loaded.IsOk());
    REQUIRE(loaded.GetError().code == DirectorDesk::Core::ErrorCode::Unsupported);
}

TEST_CASE("GLB cube example loads", "[asset][glb]") {
#ifndef DD_EXAMPLE_CUBE_GLB
    SKIP("example cube.glb path is not defined");
#else
    const std::string example = DD_EXAMPLE_CUBE_GLB;
    if (!DirectorDesk::Platform::Paths::Exists(example)) {
        SKIP("example cube.glb is not on disk");
    }
    const std::string dir = MakeCaseDir("glb-cn");
    auto bytes = DirectorDesk::Platform::Paths::ReadBinaryFile(example);
    REQUIRE(bytes.IsOk());
    const std::string dest = DirectorDesk::Platform::Paths::Join(dir, "立方体.glb");
    auto written = DirectorDesk::Platform::Paths::WriteBinaryFile(dest, bytes.Value().data(),
                                                                  bytes.Value().size());
    REQUIRE(written.IsOk());
    const auto registry = DirectorDesk::Asset::CreateDefaultRegistry();
    auto loaded = registry.Load(dest);
    REQUIRE(loaded.IsOk());
    REQUIRE_FALSE(loaded.Value().primitives.empty());
#endif
}

TEST_CASE("Corrupt GLB returns a parse error", "[asset][glb]") {
    const std::string dir = MakeCaseDir("glb-bad");
    const std::string path = DirectorDesk::Platform::Paths::Join(dir, "坏掉.glb");
    DirectorDesk::Tests::WriteTextFile(path, "glTF this is not");
    const auto registry = DirectorDesk::Asset::CreateDefaultRegistry();
    auto loaded = registry.Load(path);
    REQUIRE_FALSE(loaded.IsOk());
    REQUIRE(loaded.GetError().code == DirectorDesk::Core::ErrorCode::ParseFailure);
}
