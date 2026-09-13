// SceneLoadTests: Async scene model loads (FND-11).
#include "AppInternals.h"
#include "DirectorDesk/App/AppState.h"
#include "DirectorDesk/Core/Error.h"
#include "DirectorDesk/Core/ResultQueue.h"
#include "DirectorDesk/Platform/Worker.h"
#include "DirectorDesk/Scene/Document.h"

#include <catch2/catch_test_macros.hpp>

#include <string>

#ifndef DD_EXAMPLE_CUBE_GLB
#define DD_EXAMPLE_CUBE_GLB ""
#endif

namespace {

DirectorDesk::Scene::Node MakeNode(const std::string& id, const std::string& path,
                                    bool missing = false) {
    DirectorDesk::Scene::Node node;
    node.id = id;
    node.name = id;
    node.sourcePath = path;
    node.assetMissing = missing;
    return node;
}

} // namespace

TEST_CASE("import results do not consume scene load counters", "[app][scene-load]") {
    DirectorDesk::App::AppState state;
    DirectorDesk::App::BeginProjectGeneration(state);
    state.sceneLoadPending = 2;
    state.sceneLoadTotal = 2;

    DirectorDesk::Core::ResultQueue<DirectorDesk::Asset::ModelLoadResult> results;
    DirectorDesk::Asset::ModelLoadResult imported;
    imported.generation = state.projectGeneration;
    imported.ok = false;
    imported.error = DirectorDesk::Core::Error::Make(DirectorDesk::Core::ErrorCode::IoFailure,
                                                         "import failed", "导入失败");
    results.Push(std::move(imported));

    DirectorDesk::App::DrainLoadResults(state, nullptr, results);
    REQUIRE(state.sceneLoadPending == 2);
    REQUIRE(state.sceneLoadTotal == 2);
    REQUIRE_FALSE(state.importInProgress);
    REQUIRE(state.status == "导入失败");
}

TEST_CASE("DrainLoadResults discards stale generation", "[app][scene-load]") {
    DirectorDesk::App::AppState state;
    DirectorDesk::App::BeginProjectGeneration(state);
    REQUIRE(state.projectGeneration == 1);
    state.scene.Add(MakeNode("n1", "cube.glb"));
    state.sceneLoadPending = 1;
    state.sceneLoadTotal = 1;

    DirectorDesk::Core::ResultQueue<DirectorDesk::Asset::ModelLoadResult> results;
    DirectorDesk::Asset::ModelLoadResult stale;
    stale.generation = 0;
    stale.nodeId = "n1";
    stale.ok = true;
    results.Push(std::move(stale));

    DirectorDesk::App::DrainLoadResults(state, nullptr, results);
    REQUIRE(state.sceneLoadPending == 1);
    REQUIRE(state.sceneLoadTotal == 1);
    REQUIRE_FALSE(state.scene.Find("n1")->assetMissing);
}

TEST_CASE("failed scene load marks the node missing and clears counters", "[app][scene-load]") {
    DirectorDesk::App::AppState state;
    DirectorDesk::App::BeginProjectGeneration(state);
    state.scene.Add(MakeNode("n1", "missing.glb"));
    state.sceneLoadPending = 1;
    state.sceneLoadTotal = 1;

    DirectorDesk::Core::ResultQueue<DirectorDesk::Asset::ModelLoadResult> results;
    DirectorDesk::Asset::ModelLoadResult failed;
    failed.generation = state.projectGeneration;
    failed.nodeId = "n1";
    failed.ok = false;
    failed.error = DirectorDesk::Core::Error::Make(DirectorDesk::Core::ErrorCode::NotFound,
                                                    "missing file", "找不到模型");
    results.Push(std::move(failed));

    DirectorDesk::App::DrainLoadResults(state, nullptr, results);
    REQUIRE(state.scene.Find("n1")->assetMissing);
    REQUIRE(state.sceneLoadPending == 0);
    REQUIRE(state.sceneLoadTotal == 0);
    REQUIRE(state.status == "找不到模型");
}

TEST_CASE("QueueSceneModelLoads skips missing nodes and loads the rest", "[app][scene-load]") {
    DirectorDesk::App::AppState state;
    DirectorDesk::App::BeginProjectGeneration(state);
    state.scene.Add(MakeNode("missing", "", true));
    state.scene.Add(MakeNode("ready", "cube.glb"));

    DirectorDesk::Platform::Worker worker;
    worker.Start();
    DirectorDesk::Core::ResultQueue<DirectorDesk::Asset::ModelLoadResult> results;
    DirectorDesk::App::QueueSceneModelLoads(state, worker, results);
    worker.Shutdown();

    REQUIRE(state.sceneLoadPending == 1);
    REQUIRE(state.sceneLoadTotal == 1);
}

TEST_CASE("QueueSceneModelLoads loads example cube without marking missing", "[app][scene-load]") {
    const std::string cube = DD_EXAMPLE_CUBE_GLB;
    REQUIRE_FALSE(cube.empty());

    DirectorDesk::App::AppState state;
    DirectorDesk::App::BeginProjectGeneration(state);
    state.scene.Add(MakeNode("cube", cube));

    DirectorDesk::Platform::Worker worker;
    worker.Start();
    DirectorDesk::Core::ResultQueue<DirectorDesk::Asset::ModelLoadResult> results;
    DirectorDesk::App::QueueSceneModelLoads(state, worker, results);
    REQUIRE(state.sceneLoadPending == 1);
    worker.Shutdown();

    DirectorDesk::App::DrainLoadResults(state, nullptr, results);
    REQUIRE(state.sceneLoadPending == 0);
    REQUIRE(state.sceneLoadTotal == 0);
    REQUIRE_FALSE(state.scene.Find("cube")->assetMissing);
}
