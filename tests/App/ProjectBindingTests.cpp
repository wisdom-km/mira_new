// ProjectBindingTests: Index hash reuse on Capture / save (FND-12).
#include "AppInternals.h"
#include "DirectorDesk/App/AppState.h"
#include "DirectorDesk/App/ProjectBinding.h"
#include "DirectorDesk/App/ProjectFile.h"
#include "DirectorDesk/Asset/Library.h"
#include "DirectorDesk/Camera/CameraManager.h"
#include "DirectorDesk/Core/ResultQueue.h"
#include "DirectorDesk/Link/ShotLink.h"
#include "DirectorDesk/Platform/Paths.h"
#include "DirectorDesk/Platform/Worker.h"
#include "DirectorDesk/Scene/Document.h"
#include "DirectorDesk/Script/Document.h"

#include <catch2/catch_test_macros.hpp>

#include <chrono>
#include <string>
#include <thread>

namespace {

std::string MakeRoot(const char* name) {
    auto temp = DirectorDesk::Platform::Paths::TemporaryDirectory();
    REQUIRE(temp.IsOk());
    const std::string dir = DirectorDesk::Platform::Paths::Join(
        temp.Value(), std::string("导演台绑定_DirectorDesk/") + name);
    REQUIRE(DirectorDesk::Platform::Paths::CreateDirectories(dir).IsOk());
    return dir;
}

std::string WriteObj(const std::string& directory, const std::string& fileName,
                      const std::string& extra = {}) {
    const std::string path = DirectorDesk::Platform::Paths::Join(directory, fileName);
    REQUIRE(DirectorDesk::Platform::Paths::WriteTextFile(
                path, "v 0 0 0\nv 1 0 0\nv 0 1 0\nf 1 2 3\n" + extra)
                .IsOk());
    return path;
}

void AddImportedNode(DirectorDesk::Scene::Document& scene, const std::string& sourcePath,
                       const std::string& assetId) {
    DirectorDesk::Scene::Node node;
    node.id = "node-chair-01";
    node.name = "椅子";
    node.sourcePath = sourcePath;
    node.libraryAssetId = assetId;
    scene.Add(std::move(node));
}

} // namespace

TEST_CASE("索引有哈希则不重算", "[app][project]") {
    const std::string root = MakeRoot("cached-hash");
    const std::string projectDir = DirectorDesk::Platform::Paths::Join(root, "project");
    const std::string assetsDir = DirectorDesk::Platform::Paths::Join(projectDir, "assets");
    const std::string libraryDir = DirectorDesk::Platform::Paths::Join(root, "library");
    REQUIRE(DirectorDesk::Platform::Paths::CreateDirectories(assetsDir).IsOk());
    const std::string model = WriteObj(assetsDir, "椅子.obj");
    const std::string projectPath = DirectorDesk::Platform::Paths::Join(projectDir, "saved.ddproj");

    DirectorDesk::Asset::Library library;
    REQUIRE(library.Open(libraryDir).IsOk());
    auto imported = library.Import(model, DirectorDesk::Asset::AssetOrigin::User);
    REQUIRE(imported.IsOk());

    DirectorDesk::App::ProjectFile::ResetSha256FileReadCount();
    auto hashed = DirectorDesk::App::ProjectFile::Sha256File(model);
    REQUIRE(hashed.IsOk());
    REQUIRE(DirectorDesk::App::ProjectFile::Sha256FileReadCount() == 1);
    REQUIRE(library.RecordContentHash(model, imported.Value().id, hashed.Value()));

    DirectorDesk::Scene::Document scene;
    AddImportedNode(scene, model, imported.Value().id);
    DirectorDesk::Camera::CameraManager cameras;
    DirectorDesk::Link::Table links;
    DirectorDesk::Script::Document script;

    REQUIRE(DirectorDesk::App::CollectUncachedSourcePaths(scene, library).empty());
    DirectorDesk::App::ProjectFile::ResetSha256FileReadCount();
    auto captured = DirectorDesk::App::CaptureProject("proj-hash", "咖啡馆", projectPath, scene,
                                                       cameras, links, script, library, {});
    REQUIRE(DirectorDesk::App::ProjectFile::Sha256FileReadCount() == 0);
    REQUIRE_FALSE(captured.assets.empty());
    REQUIRE(captured.assets.front().sha256 == hashed.Value());

    DirectorDesk::Asset::Library reopened;
    REQUIRE(reopened.Open(libraryDir).IsOk());
    std::string cached;
    REQUIRE(reopened.TryCachedHash(model, imported.Value().id, cached));
    REQUIRE(cached == hashed.Value());

    REQUIRE(WriteObj(assetsDir, "椅子.obj", "v 0 0 1\n") == model);
    REQUIRE_FALSE(DirectorDesk::App::CollectUncachedSourcePaths(scene, library).empty());
    DirectorDesk::App::ProjectFile::ResetSha256FileReadCount();
    auto recaptured = DirectorDesk::App::CaptureProject("proj-hash", "咖啡馆", projectPath, scene,
                                                        cameras, links, script, library, {});
    REQUIRE(DirectorDesk::App::ProjectFile::Sha256FileReadCount() >= 1);
    REQUIRE(recaptured.assets.front().sha256 != hashed.Value());
}

TEST_CASE("save hashes in background and queues a second save", "[app][project]") {
    const std::string root = MakeRoot("async-save");
    const std::string projectDir = DirectorDesk::Platform::Paths::Join(root, "project");
    const std::string assetsDir = DirectorDesk::Platform::Paths::Join(projectDir, "assets");
    const std::string libraryDir = DirectorDesk::Platform::Paths::Join(root, "library");
    REQUIRE(DirectorDesk::Platform::Paths::CreateDirectories(assetsDir).IsOk());
    const std::string model = WriteObj(assetsDir, "椅子.obj");
    const std::string projectPath = DirectorDesk::Platform::Paths::Join(projectDir, "saved.ddproj");

    DirectorDesk::App::AppState state;
    REQUIRE(state.library.Open(libraryDir).IsOk());
    auto imported = state.library.Import(model, DirectorDesk::Asset::AssetOrigin::User);
    REQUIRE(imported.IsOk());
    AddImportedNode(state.scene, model, imported.Value().id);
    state.projectName = "咖啡馆";
    DirectorDesk::App::BeginProjectGeneration(state);

    DirectorDesk::Platform::Worker worker;
    worker.Start();
    DirectorDesk::Core::ResultQueue<DirectorDesk::App::SaveHashJobResult> hashResults;
    DirectorDesk::App::ProjectFile::ResetSha256FileReadCount();

    const auto first =
        DirectorDesk::App::RequestSaveProject(state, projectPath, &worker, &hashResults);
    REQUIRE(first == DirectorDesk::App::SaveProjectStatus::Deferred);
    REQUIRE(state.projectSaveInProgress);
    REQUIRE(state.status == "正在校验资产");

    const auto second =
        DirectorDesk::App::RequestSaveProject(state, projectPath, &worker, &hashResults);
    REQUIRE(second == DirectorDesk::App::SaveProjectStatus::Deferred);
    REQUIRE(state.projectSaveQueued);

    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    while (state.projectSaveInProgress && std::chrono::steady_clock::now() < deadline) {
        DirectorDesk::App::DrainSaveHashResults(state, hashResults);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    REQUIRE_FALSE(state.projectSaveInProgress);
    REQUIRE(DirectorDesk::Platform::Paths::Exists(projectPath));
    REQUIRE(DirectorDesk::App::ProjectFile::Sha256FileReadCount() >= 1);

    if (state.projectSaveQueued) {
        state.projectSaveQueued = false;
        DirectorDesk::App::ProjectFile::ResetSha256FileReadCount();
        const auto queued =
            DirectorDesk::App::RequestSaveProject(state, state.projectSavePendingPath, &worker,
                                                   &hashResults);
        REQUIRE(queued == DirectorDesk::App::SaveProjectStatus::Saved);
        REQUIRE(DirectorDesk::App::ProjectFile::Sha256FileReadCount() == 0);
    }

    worker.Shutdown();
}
