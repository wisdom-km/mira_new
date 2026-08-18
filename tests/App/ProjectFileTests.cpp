#include "DirectorDesk/App/ProjectBinding.h"
#include "DirectorDesk/App/ProjectFile.h"
#include "DirectorDesk/Asset/Library.h"
#include "DirectorDesk/Camera/CameraManager.h"
#include "DirectorDesk/Link/ShotLink.h"
#include "DirectorDesk/Platform/Paths.h"
#include "DirectorDesk/Scene/Document.h"
#include "DirectorDesk/Script/Document.h"

#include <catch2/catch_test_macros.hpp>

#include <string>

namespace {

std::string MakeRoot(const char* name) {
    auto temp = DirectorDesk::Platform::Paths::TemporaryDirectory();
    REQUIRE(temp.IsOk());
    const std::string dir = DirectorDesk::Platform::Paths::Join(
        temp.Value(), std::string("导演台工程_DirectorDesk/") + name);
    REQUIRE(DirectorDesk::Platform::Paths::CreateDirectories(dir).IsOk());
    return dir;
}

std::string WriteObj(const std::string& directory, const std::string& fileName) {
    const std::string path = DirectorDesk::Platform::Paths::Join(directory, fileName);
    REQUIRE(DirectorDesk::Platform::Paths::WriteTextFile(path, "v 0 0 0\nv 1 0 0\nv 0 1 0\nf 1 2 3\n")
                .IsOk());
    return path;
}

DirectorDesk::App::ProjectSnapshot SampleSnapshot(const std::string& projectDir) {
    const std::string models = DirectorDesk::Platform::Paths::Join(projectDir, "assets");
    REQUIRE(DirectorDesk::Platform::Paths::CreateDirectories(models).IsOk());
    const std::string model = WriteObj(models, "椅子.obj");
    auto hash = DirectorDesk::App::ProjectFile::Sha256File(model);
    REQUIRE(hash.IsOk());

    DirectorDesk::App::ProjectSnapshot snapshot;
    snapshot.projectId = "proj-test-1";
    snapshot.name = "咖啡馆短片";
    snapshot.script.kind = DirectorDesk::App::StoredPathKind::ProjectRelative;
    snapshot.script.value = "script/story.md";
    const std::string scriptDir = DirectorDesk::Platform::Paths::Join(projectDir, "script");
    REQUIRE(DirectorDesk::Platform::Paths::CreateDirectories(scriptDir).IsOk());
    REQUIRE(DirectorDesk::Platform::Paths::WriteTextFile(
                DirectorDesk::Platform::Paths::Join(scriptDir, "story.md"),
                "## [scene:scene-cafe] 咖啡馆\n\n### [shot:shot-cafe-001] 进门\n\n")
                .IsOk());

    DirectorDesk::App::ProjectAssetRef asset;
    asset.refId = "assetref-chair";
    asset.source = DirectorDesk::App::ProjectAssetSource::Project;
    asset.path = "assets/椅子.obj";
    asset.sha256 = hash.Value();
    snapshot.assets.push_back(asset);

    DirectorDesk::App::ProjectNode node;
    node.id = "node-chair-01";
    node.name = "椅子";
    node.assetRef = "assetref-chair";
    snapshot.nodes.push_back(node);

    DirectorDesk::App::ProjectCamera camera;
    camera.id = "cam-1";
    camera.name = "主镜头";
    camera.preset = "front";
    snapshot.cameras.push_back(camera);
    snapshot.activeCamera = "cam-1";
    snapshot.shotLinks.push_back({"shot-cafe-001", "cam-1"});
    snapshot.collapsedScenes.push_back("scene-cafe");
    snapshot.lightingPreset = "warm";
    return snapshot;
}

} // namespace

TEST_CASE("Project file version 1 round-trips Chinese names", "[project]") {
    const std::string dir = MakeRoot("roundtrip");
    const std::string path = DirectorDesk::Platform::Paths::Join(dir, "工程.ddproj");
    auto snapshot = SampleSnapshot(dir);
    REQUIRE(DirectorDesk::App::ProjectFile::Save(path, snapshot).IsOk());

    auto loaded = DirectorDesk::App::ProjectFile::Load(path);
    REQUIRE(loaded.IsOk());
    REQUIRE(loaded.Value().name == "咖啡馆短片");
    REQUIRE(loaded.Value().nodes.front().name == "椅子");
    REQUIRE(loaded.Value().assets.front().path == "assets/椅子.obj");
    REQUIRE(loaded.Value().shotLinks.front().shotId == "shot-cafe-001");
    REQUIRE(loaded.Value().collapsedScenes.front() == "scene-cafe");
    REQUIRE(loaded.Value().lightingPreset == "warm");
}

TEST_CASE("Relative assets survive moving the project directory", "[project]") {
    const std::string dir = MakeRoot("move-src");
    const std::string path = DirectorDesk::Platform::Paths::Join(dir, "project.ddproj");
    REQUIRE(DirectorDesk::App::ProjectFile::Save(path, SampleSnapshot(dir)).IsOk());

    const std::string dest = MakeRoot("move-dst/子目录");
    const std::string copiedProj = DirectorDesk::Platform::Paths::Join(dest, "project.ddproj");
    auto bytes = DirectorDesk::Platform::Paths::ReadBinaryFile(path);
    REQUIRE(bytes.IsOk());
    REQUIRE(DirectorDesk::Platform::Paths::WriteBinaryFile(copiedProj, bytes.Value().data(),
                                                           bytes.Value().size())
                .IsOk());
    const std::string destAssets = DirectorDesk::Platform::Paths::Join(dest, "assets");
    REQUIRE(DirectorDesk::Platform::Paths::CreateDirectories(destAssets).IsOk());
    auto modelBytes = DirectorDesk::Platform::Paths::ReadBinaryFile(
        DirectorDesk::Platform::Paths::Join(dir, "assets/椅子.obj"));
    REQUIRE(modelBytes.IsOk());
    REQUIRE(DirectorDesk::Platform::Paths::WriteBinaryFile(
                DirectorDesk::Platform::Paths::Join(destAssets, "椅子.obj"),
                modelBytes.Value().data(), modelBytes.Value().size())
                .IsOk());

    auto loaded = DirectorDesk::App::ProjectFile::Load(copiedProj);
    REQUIRE(loaded.IsOk());
    DirectorDesk::Scene::Document scene;
    DirectorDesk::Camera::CameraManager cameras;
    DirectorDesk::Link::Table links;
    DirectorDesk::Script::Document script;
    DirectorDesk::Asset::Library library;
    std::vector<std::string> diagnostics;
    REQUIRE(DirectorDesk::App::HydrateProject(loaded.Value(), dest, scene, cameras, links, script,
                                              library, diagnostics)
                .IsOk());
    REQUIRE(scene.Find("node-chair-01") != nullptr);
    REQUIRE_FALSE(scene.Find("node-chair-01")->assetMissing);
    REQUIRE(cameras.Find("cam-1") != nullptr);
    REQUIRE(*links.CameraForShot("shot-cafe-001") == "cam-1");
}

TEST_CASE("Missing assets and dangling links do not fail open", "[project]") {
    const std::string dir = MakeRoot("missing");
    auto snapshot = SampleSnapshot(dir);
    snapshot.assets.front().path = "assets/不存在.obj";
    snapshot.shotLinks.push_back({"shot-ghost", "cam-missing"});
    const std::string path = DirectorDesk::Platform::Paths::Join(dir, "project.ddproj");
    REQUIRE(DirectorDesk::App::ProjectFile::Save(path, snapshot).IsOk());
    auto loaded = DirectorDesk::App::ProjectFile::Load(path);
    REQUIRE(loaded.IsOk());
    DirectorDesk::Scene::Document scene;
    DirectorDesk::Camera::CameraManager cameras;
    DirectorDesk::Link::Table links;
    DirectorDesk::Script::Document script;
    DirectorDesk::Asset::Library library;
    std::vector<std::string> diagnostics;
    REQUIRE(DirectorDesk::App::HydrateProject(loaded.Value(), dir, scene, cameras, links, script,
                                              library, diagnostics)
                .IsOk());
    REQUIRE(scene.Find("node-chair-01")->assetMissing);
    REQUIRE(links.CameraForShot("shot-ghost") != nullptr);
}

TEST_CASE("Corrupt or newer project files are rejected", "[project]") {
    const std::string dir = MakeRoot("reject");
    const std::string path = DirectorDesk::Platform::Paths::Join(dir, "broken.ddproj");
    REQUIRE(DirectorDesk::Platform::Paths::WriteTextFile(path, "{not-json").IsOk());
    REQUIRE_FALSE(DirectorDesk::App::ProjectFile::Load(path).IsOk());

    auto snapshot = SampleSnapshot(dir);
    auto text = DirectorDesk::App::ProjectFile::Serialize(snapshot);
    REQUIRE(text.IsOk());
    std::string newer = text.Value();
    const auto pos = newer.find("\"formatVersion\": 1");
    REQUIRE(pos != std::string::npos);
    newer.replace(pos, 18, "\"formatVersion\": 2");
    const std::string newerPath = DirectorDesk::Platform::Paths::Join(dir, "newer.ddproj");
    REQUIRE(DirectorDesk::Platform::Paths::WriteTextFile(newerPath, newer).IsOk());
    auto loaded = DirectorDesk::App::ProjectFile::Load(newerPath);
    REQUIRE_FALSE(loaded.IsOk());
    REQUIRE(loaded.GetError().userMessage.find("升级") != std::string::npos);
}

TEST_CASE("Unknown fields are ignored and cycles are rejected", "[project]") {
    const std::string dir = MakeRoot("fields");
    auto snapshot = SampleSnapshot(dir);
    auto text = DirectorDesk::App::ProjectFile::Serialize(snapshot);
    REQUIRE(text.IsOk());
    std::string extra = text.Value();
    extra.insert(extra.rfind('}'), ",\"futureField\": true");
    auto parsed = DirectorDesk::App::ProjectFile::Parse(extra, dir);
    REQUIRE(parsed.IsOk());

    snapshot.nodes.push_back(snapshot.nodes.front());
    snapshot.nodes.back().id = "node-b";
    snapshot.nodes.back().parent = "node-chair-01";
    snapshot.nodes.front().parent = "node-b";
    auto cyclic = DirectorDesk::App::ProjectFile::Serialize(snapshot);
    REQUIRE(cyclic.IsOk());
    REQUIRE_FALSE(DirectorDesk::App::ProjectFile::Parse(cyclic.Value(), dir).IsOk());
}

TEST_CASE("Interrupted save leaves the original project intact", "[project]") {
    const std::string dir = MakeRoot("interrupt");
    const std::string path = DirectorDesk::Platform::Paths::Join(dir, "project.ddproj");
    REQUIRE(DirectorDesk::App::ProjectFile::Save(path, SampleSnapshot(dir)).IsOk());
    REQUIRE(DirectorDesk::Platform::Paths::WriteTextFile(path + ".ddtmp", "{incomplete").IsOk());
    auto loaded = DirectorDesk::App::ProjectFile::Load(path);
    REQUIRE(loaded.IsOk());
    REQUIRE(loaded.Value().projectId == "proj-test-1");
}

TEST_CASE("Capture and hydrate restore cameras and collapse state", "[project]") {
    const std::string dir = MakeRoot("hydrate");
    auto snapshot = SampleSnapshot(dir);
    DirectorDesk::Scene::Document scene;
    DirectorDesk::Camera::CameraManager cameras;
    DirectorDesk::Link::Table links;
    DirectorDesk::Script::Document script;
    DirectorDesk::Asset::Library library;
    std::vector<std::string> diagnostics;
    REQUIRE(DirectorDesk::App::HydrateProject(snapshot, dir, scene, cameras, links, script, library,
                                              diagnostics)
                .IsOk());
    const std::string path = DirectorDesk::Platform::Paths::Join(dir, "saved.ddproj");
    auto captured = DirectorDesk::App::CaptureProject(
        snapshot.projectId, snapshot.name, path, scene, cameras, links, script, library,
        snapshot.collapsedScenes);
    REQUIRE(captured.collapsedScenes == snapshot.collapsedScenes);
    REQUIRE(captured.activeCamera == "cam-1");
    REQUIRE(captured.shotLinks.size() == 1);
}

#ifdef DD_EXAMPLE_CAFE_PROJECT
TEST_CASE("Shipped cafe example project opens with Chinese paths nearby", "[project][example]") {
    auto loaded = DirectorDesk::App::ProjectFile::Load(DD_EXAMPLE_CAFE_PROJECT);
    REQUIRE(loaded.IsOk());
    REQUIRE(loaded.Value().name == "咖啡馆示例");
    REQUIRE(loaded.Value().script.value == "scripts/cafe.md");
    REQUIRE(loaded.Value().assets.front().path == "models/cube.glb");

    const std::string projectDir = DirectorDesk::Platform::Paths::Parent(DD_EXAMPLE_CAFE_PROJECT);
    DirectorDesk::Scene::Document scene;
    DirectorDesk::Camera::CameraManager cameras;
    DirectorDesk::Link::Table links;
    DirectorDesk::Script::Document script;
    DirectorDesk::Asset::Library library;
    std::vector<std::string> diagnostics;
    REQUIRE(DirectorDesk::App::HydrateProject(loaded.Value(), projectDir, scene, cameras, links,
                                              script, library, diagnostics)
                .IsOk());
    REQUIRE(scene.Find("node-cube-01") != nullptr);
    REQUIRE_FALSE(scene.Find("node-cube-01")->assetMissing);
    REQUIRE(cameras.Find("cam-main") != nullptr);
    REQUIRE(*links.CameraForShot("shot-cafe-001") == "cam-main");
    REQUIRE(script.Text().find("咖啡馆") != std::string::npos);
}
#endif
