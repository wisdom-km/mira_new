// DispatchTests: App Command semantics without a window (FND-10 / FND-13).
#include "AppInternals.h"
#include "DirectorDesk/App/AppState.h"
#include "DirectorDesk/App/CommandDispatch.h"
#include "DirectorDesk/Asset/Library.h"
#include "DirectorDesk/Platform/Paths.h"
#include "DirectorDesk/Scene/Document.h"
#include "DirectorDesk/Script/Document.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <string>

namespace {

DirectorDesk::App::DispatchServices MakeServices() {
    return {};
}

std::string MakeLibDir(const char* name) {
    auto temp = DirectorDesk::Platform::Paths::TemporaryDirectory();
    REQUIRE(temp.IsOk());
    const std::string dir = DirectorDesk::Platform::Paths::Join(
        temp.Value(), std::string("导演台分发_DirectorDesk/") + name);
    REQUIRE(DirectorDesk::Platform::Paths::CreateDirectories(dir).IsOk());
    return dir;
}

} // namespace

TEST_CASE("Dispatch InsertShot appends after existing shots", "[app][dispatch]") {
    DirectorDesk::App::AppState state;
    REQUIRE(state.script
                .LoadFromText("## [scene:s1] 场\n\n### [shot:shot-a] 过肩\n\n窗。\n\n### "
                              "[shot:shot-b] 特写\n\n手。\n")
                .IsOk());
    REQUIRE(state.script.PublishedSnapshot().scenes[0].shots.size() == 2);
    auto services = MakeServices();
    DirectorDesk::App::Dispatch(state, DirectorDesk::Core::InsertShotCommand{}, services);
    REQUIRE(state.script.PublishedSnapshot().scenes[0].shots.size() == 3);
    REQUIRE(state.script.PublishedSnapshot().scenes[0].shots[0].id == "shot-a");
    REQUIRE(state.script.PublishedSnapshot().scenes[0].shots[1].id == "shot-b");
    REQUIRE(state.script.PublishedSnapshot().scenes[0].shots[2].id == state.script.SelectedShotId());
    REQUIRE(state.projectDirty);
    REQUIRE(state.status == "已添加镜头");
}

TEST_CASE("Dispatch InsertShot afterShotId inserts after that shot", "[app][dispatch]") {
    DirectorDesk::App::AppState state;
    REQUIRE(state.script
                .LoadFromText("## [scene:s1] 场\n\n### [shot:shot-a] 过肩\n\n窗。\n\n### "
                              "[shot:shot-b] 特写\n\n手。\n")
                .IsOk());
    auto services = MakeServices();
    DirectorDesk::App::Dispatch(state, DirectorDesk::Core::InsertShotCommand{"shot-a"}, services);
    REQUIRE(state.script.PublishedSnapshot().scenes[0].shots.size() == 3);
    REQUIRE(state.script.PublishedSnapshot().scenes[0].shots[0].id == "shot-a");
    REQUIRE(state.script.PublishedSnapshot().scenes[0].shots[1].id == state.script.SelectedShotId());
    REQUIRE(state.script.PublishedSnapshot().scenes[0].shots[2].id == "shot-b");
    REQUIRE(state.status == "已添加镜头");
    REQUIRE(state.projectDirty);
}

TEST_CASE("Dispatch DeleteShot removes the shot and its link", "[app][dispatch]") {
    DirectorDesk::App::AppState state;
    REQUIRE(state.script
                .LoadFromText("## [scene:s1] 场\n\n### [shot:shot-a] 过肩\n\n### [shot:shot-b] "
                              "特写\n\n")
                .IsOk());
    state.script.SelectShot("shot-a");
    state.selectionKind = "shot";
    state.selectionId = "shot-a";
    state.links.Set("shot-a", "cam-1");
    auto services = MakeServices();
    DirectorDesk::App::Dispatch(state, DirectorDesk::Core::DeleteShotCommand{"shot-a"}, services);
    REQUIRE(state.script.PublishedSnapshot().scenes[0].shots.size() == 1);
    REQUIRE(state.script.PublishedSnapshot().scenes[0].shots[0].id == "shot-b");
    REQUIRE(state.links.CameraForShot("shot-a") == nullptr);
    REQUIRE(state.projectDirty);
    REQUIRE(state.status == "已删除镜头");
}

TEST_CASE("Dispatch BindShotToNewCamera adds a camera and links the shot", "[app][dispatch]") {
    DirectorDesk::App::AppState state;
    REQUIRE(state.script.LoadFromText("## [scene:s1] 场\n\n### [shot:shot-a] 过肩\n\n").IsOk());
    state.script.SelectShot("shot-a");
    const std::size_t before = state.cameras.Cameras().size();
    auto services = MakeServices();
    DirectorDesk::App::Dispatch(state, DirectorDesk::Core::BindShotToNewCameraCommand{"shot-a"},
                                services);
    REQUIRE(state.cameras.Cameras().size() == before + 1);
    REQUIRE(state.links.CameraForShot("shot-a") != nullptr);
    REQUIRE(*state.links.CameraForShot("shot-a") == state.cameras.SelectedId());
    REQUIRE(state.projectDirty);
    REQUIRE(state.selectionKind == "shot");
    REQUIRE(state.status.find("新建并绑定") != std::string::npos);
}

TEST_CASE("Dispatch SelectExportResolution rejects unknown ids", "[app][dispatch]") {
    DirectorDesk::App::AppState state;
    REQUIRE(state.exportResolutionId == "1080p");
    auto services = MakeServices();
    DirectorDesk::App::Dispatch(state, DirectorDesk::Core::SelectExportResolutionCommand{"4k-ultra"},
                                services);
    REQUIRE(state.exportResolutionId == "1080p");
    REQUIRE(state.status == "未知导出分辨率");
    DirectorDesk::App::Dispatch(state, DirectorDesk::Core::SelectExportResolutionCommand{"2k"},
                                services);
    REQUIRE(state.exportResolutionId == "2k");
    REQUIRE(state.status == "导出分辨率 2k");
}

TEST_CASE("Dispatch SetWorkspaceMode ignores unknown ids", "[app][dispatch]") {
    DirectorDesk::App::AppState state;
    REQUIRE(state.workspaceModeId == "shoot");
    auto services = MakeServices();
    DirectorDesk::App::Dispatch(state, DirectorDesk::Core::SetWorkspaceModeCommand{"live"},
                                services);
    REQUIRE(state.workspaceModeId == "shoot");
    REQUIRE_FALSE(state.layoutRebuildRequested);
    REQUIRE(state.status == "未知工作区模式");
    DirectorDesk::App::Dispatch(state, DirectorDesk::Core::SetWorkspaceModeCommand{"review"},
                                services);
    REQUIRE(state.workspaceModeId == "review");
    REQUIRE(state.layoutRebuildRequested);
}

TEST_CASE("Dispatch RemoveLibraryAsset does not mark the project dirty", "[app][dispatch]") {
    DirectorDesk::App::AppState state;
    const std::string root = MakeLibDir("remove");
    REQUIRE(state.library.Open(root).IsOk());
    const std::string model = DirectorDesk::Platform::Paths::Join(root, "cube.obj");
    REQUIRE(DirectorDesk::Platform::Paths::WriteTextFile(model, "v 0 0 0\nv 1 0 0\nv 0 1 0\nf 1 2 3\n")
                .IsOk());
    auto imported = state.library.Import(model, DirectorDesk::Asset::AssetOrigin::User);
    REQUIRE(imported.IsOk());
    state.projectDirty = false;
    auto services = MakeServices();
    DirectorDesk::App::Dispatch(state, DirectorDesk::Core::RemoveLibraryAssetCommand{imported.Value().id},
                                services);
    REQUIRE(state.library.Find(imported.Value().id) == nullptr);
    REQUIRE_FALSE(state.projectDirty);
    REQUIRE(state.status == "已从资源库删除");
}

TEST_CASE("Dispatch DuplicateNode shares gpuModelId and DeleteNode keeps the remaining ref",
          "[app][dispatch]") {
    DirectorDesk::App::AppState state;
    DirectorDesk::Scene::Node chair;
    chair.id = "node-1";
    chair.name = "椅子";
    chair.gpuModelId = 42;
    state.scene.Add(std::move(chair));
    state.gpuModelRefs[42] = 1;
    state.selectionKind = "node";
    state.selectionId = "node-1";
    state.selectionLabel = "椅子";
    auto services = MakeServices();

    DirectorDesk::App::Dispatch(state, DirectorDesk::Core::DuplicateNodeCommand{"node-1"}, services);
    REQUIRE(state.scene.Nodes().size() == 2);
    const std::string copyId = state.scene.SelectedId();
    const DirectorDesk::Scene::Node* copy = state.scene.Find(copyId);
    REQUIRE(copy != nullptr);
    REQUIRE(copyId != "node-1");
    REQUIRE(copy->gpuModelId == 42);
    REQUIRE(copy->transform.position.x == Catch::Approx(0.5f));
    REQUIRE(state.gpuModelRefs[42] == 2);
    REQUIRE(state.selectionKind == "node");
    REQUIRE(state.selectionId == copyId);
    REQUIRE(state.status.find("已复制为") != std::string::npos);
    REQUIRE(state.projectDirty);

    DirectorDesk::App::Dispatch(state, DirectorDesk::Core::DeleteNodeCommand{"node-1"}, services);
    REQUIRE(state.scene.Find("node-1") == nullptr);
    REQUIRE(state.scene.Find(copyId) != nullptr);
    REQUIRE(state.scene.Find(copyId)->gpuModelId == 42);
    REQUIRE(state.gpuModelRefs[42] == 1);
    REQUIRE(state.status.find("已删除") != std::string::npos);
}

TEST_CASE("Dispatch SetNodeVisible hides the node from the scene view", "[app][dispatch]") {
    DirectorDesk::App::AppState state;
    DirectorDesk::Scene::Node chair;
    chair.id = "node-1";
    chair.name = "椅子";
    chair.gpuModelId = 9;
    state.scene.Add(std::move(chair));
    auto services = MakeServices();
    DirectorDesk::App::Dispatch(state, DirectorDesk::Core::SetNodeVisibleCommand{"node-1", false},
                                services);
    REQUIRE_FALSE(state.scene.Find("node-1")->visible);
    REQUIRE(state.projectDirty);
    REQUIRE(state.status.empty());
    const auto view = DirectorDesk::App::BuildSceneView(state.scene, state.cameras.CurrentLight(),
                                                          false, false);
    REQUIRE(view.instances.empty());
}
