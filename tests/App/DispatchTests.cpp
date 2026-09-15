// DispatchTests: App Command semantics without a window (FND-10 / FND-13).
#include "AppInternals.h"
#include "DirectorDesk/App/AppState.h"
#include "DirectorDesk/App/CommandDispatch.h"
#include "DirectorDesk/App/ViewStateBuilder.h"
#include "DirectorDesk/Asset/Library.h"
#include "DirectorDesk/Camera/CameraManager.h"
#include "DirectorDesk/Export/ShotExport.h"
#include "DirectorDesk/Platform/Paths.h"
#include "DirectorDesk/Scene/Document.h"
#include "DirectorDesk/Script/Document.h"
#include "DirectorDesk/UI/IPanel.h"

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
    REQUIRE(state.script.PublishedSnapshot().scenes[0].shots[2].id ==
            state.script.SelectedShotId());
    REQUIRE(state.selectionKind == "shot");
    REQUIRE(state.selectionId == state.script.SelectedShotId());
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
    REQUIRE(state.script.PublishedSnapshot().scenes[0].shots[1].id ==
            state.script.SelectedShotId());
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
    DirectorDesk::App::Dispatch(
        state, DirectorDesk::Core::SelectExportResolutionCommand{"4k-ultra"}, services);
    REQUIRE(state.exportResolutionId == "1080p");
    REQUIRE(state.status == "未知导出分辨率");
    DirectorDesk::App::Dispatch(state, DirectorDesk::Core::SelectExportResolutionCommand{"2k"},
                                services);
    REQUIRE(state.exportResolutionId == "2k");
    REQUIRE(state.status == "导出分辨率 2k");
}

TEST_CASE("Dispatch export defaults write UserSettings", "[app][dispatch]") {
    DirectorDesk::App::AppState state;
    auto services = MakeServices();
    REQUIRE(state.userSettings.exportTransparent);
    REQUIRE(state.userSettings.exportResolutionId == "1080p");
    DirectorDesk::App::Dispatch(state, DirectorDesk::Core::SetExportTransparentCommand{false},
                                services);
    REQUIRE_FALSE(state.exportTransparent);
    REQUIRE_FALSE(state.userSettings.exportTransparent);
    DirectorDesk::App::Dispatch(state, DirectorDesk::Core::SelectExportResolutionCommand{"2k"},
                                services);
    REQUIRE(state.exportResolutionId == "2k");
    REQUIRE(state.userSettings.exportResolutionId == "2k");
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
    REQUIRE(
        DirectorDesk::Platform::Paths::WriteTextFile(model, "v 0 0 0\nv 1 0 0\nv 0 1 0\nf 1 2 3\n")
            .IsOk());
    auto imported = state.library.Import(model, DirectorDesk::Asset::AssetOrigin::User);
    REQUIRE(imported.IsOk());
    state.projectDirty = false;
    auto services = MakeServices();
    DirectorDesk::App::Dispatch(
        state, DirectorDesk::Core::RemoveLibraryAssetCommand{imported.Value().id}, services);
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

    DirectorDesk::App::Dispatch(state, DirectorDesk::Core::DuplicateNodeCommand{"node-1"},
                                services);
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
    const auto view =
        DirectorDesk::App::BuildSceneView(state.scene, state.cameras.CurrentLight(), false, false);
    REQUIRE(view.instances.empty());
}

TEST_CASE("BuildViewState maps selectionKind strings to enum", "[app][dispatch]") {
    DirectorDesk::App::AppState state;
    REQUIRE(
        state.script.LoadFromText("## [scene:s1] 场\n\n### [shot:shot-a] 过肩\n\n窗。\n").IsOk());
    DirectorDesk::UI::AppViewState view;
    DirectorDesk::App::FrameStrings frame;
    state.selectionKind = "shot";
    state.selectionId = "shot-a";
    DirectorDesk::App::BuildViewState(state, view, frame);
    REQUIRE(view.selectionKindEnum == DirectorDesk::UI::SelectionKind::Shot);

    DirectorDesk::Asset::LibraryAsset asset;
    asset.id = "prop-1";
    asset.name = "椅子";
    asset.sourcePath = "chair.obj";
    asset.format = "obj";
    const std::string libDir = MakeLibDir("view-enum");
    REQUIRE(state.library.Open(libDir).IsOk());
    REQUIRE(state.library.Upsert(asset).IsOk());
    state.selectionKind = "asset";
    state.selectionId = "prop-1";
    DirectorDesk::App::BuildViewState(state, view, frame);
    REQUIRE(view.selectionKindEnum == DirectorDesk::UI::SelectionKind::Asset);

    state.selectionKind = "none";
    state.selectionId.clear();
    DirectorDesk::App::BuildViewState(state, view, frame);
    REQUIRE(view.selectionKindEnum == DirectorDesk::UI::SelectionKind::None);
}

TEST_CASE("Export log shotId locates the matching shot among title twins", "[app][dispatch]") {
    std::vector<DirectorDesk::UI::StoryboardCardView> cards(2);
    cards[0].kindEnum = DirectorDesk::UI::CardKind::Shot;
    cards[0].shotId = "shot-a";
    cards[0].title = "过肩";
    cards[1].kindEnum = DirectorDesk::UI::CardKind::Shot;
    cards[1].shotId = "shot-b";
    cards[1].title = "过肩";
    DirectorDesk::UI::ExportLogView first;
    first.shotId = "shot-a";
    first.shotTitle = "过肩";
    DirectorDesk::UI::ExportLogView second;
    second.shotId = "shot-b";
    second.shotTitle = "过肩";
    REQUIRE(DirectorDesk::UI::FindShotCardById(&cards, first.shotId)->shotId == "shot-a");
    REQUIRE(DirectorDesk::UI::FindShotCardById(&cards, second.shotId)->shotId == "shot-b");
    REQUIRE(DirectorDesk::UI::FindShotCardById(&cards, {}) == nullptr);
}

TEST_CASE("Dispatch SetShotMeta rewrites shot metadata", "[app][dispatch]") {
    DirectorDesk::App::AppState state;
    REQUIRE(state.script
                .LoadFromText("## [scene:s1] 场\n### [shot:shot-a] 过肩\n> 景别: 中景\n\n窗。\n")
                .IsOk());
    auto services = MakeServices();
    DirectorDesk::App::Dispatch(
        state, DirectorDesk::Core::SetShotMetaCommand{"shot-a", "景别", "特写"}, services);
    REQUIRE(state.script.PublishedSnapshot().scenes[0].shots[0].meta[0].value == "特写");
    REQUIRE(state.script.Text().find("> 景别: 特写") != std::string::npos);
}

TEST_CASE("Dispatch ImportStoryboard appends scenes", "[app][dispatch]") {
    DirectorDesk::App::AppState state;
    REQUIRE(state.script.LoadFromText("## [scene:s1] 场\n### [shot:shot-a] 旧\n正文\n").IsOk());
    auto temp = DirectorDesk::Platform::Paths::TemporaryDirectory();
    REQUIRE(temp.IsOk());
    const std::string dir =
        DirectorDesk::Platform::Paths::Join(temp.Value(), "导演台导入_DirectorDesk");
    REQUIRE(DirectorDesk::Platform::Paths::CreateDirectories(dir).IsOk());
    const std::string path = DirectorDesk::Platform::Paths::Join(dir, "import.json");
    const char* json =
        R"({"format":"DirectorDeskStoryboardImport","formatVersion":1,"title":"导入",
            "scenes":[{"id":"scene-in","title":"新场","shots":[{"id":"shot-in","title":"新镜","body":"走。","meta":{"景别":"中景"}}]}]})";
    REQUIRE(DirectorDesk::Platform::Paths::WriteTextFile(path, json).IsOk());
    auto services = MakeServices();
    DirectorDesk::App::Dispatch(
        state, DirectorDesk::Core::ImportStoryboardFromPathCommand{path, "append"}, services);
    REQUIRE(state.script.PublishedSnapshot().scenes.size() == 2);
    REQUIRE(state.script.PublishedSnapshot().scenes[1].id == "scene-in");
    REQUIRE(state.script.PublishedSnapshot().scenes[1].shots[0].meta[0].value == "中景");
    REQUIRE(state.status.find("已导入") != std::string::npos);
}

TEST_CASE("Dispatch AddLibraryAssetToScene rejects skill assets", "[app][dispatch]") {
    DirectorDesk::App::AppState state;
    const std::string dir = MakeLibDir("skill-reject");
    REQUIRE(state.library.Open(dir).IsOk());
    DirectorDesk::Asset::LibraryAsset skill;
    skill.id = "storyboard-skill";
    skill.name = "分镜 Skill";
    skill.sourcePath = DirectorDesk::Platform::Paths::Join(dir, "SKILL.md");
    skill.format = "skill";
    skill.sourceExists = true;
    REQUIRE(DirectorDesk::Platform::Paths::WriteTextFile(skill.sourcePath, "# Skill\n").IsOk());
    REQUIRE(state.library.Upsert(skill).IsOk());
    auto services = MakeServices();
    DirectorDesk::App::Dispatch(state, DirectorDesk::Core::AddLibraryAssetToSceneCommand{skill.id},
                                services);
    REQUIRE(state.status == "Skill 不是模型");
    REQUIRE(state.scene.Nodes().empty());
}

TEST_CASE("BuildViewState projectIsEmpty ignores empty script snapshot and placeholder cube",
          "[app][dispatch][uic43]") {
    DirectorDesk::App::AppState state;
    REQUIRE(state.script.HasPublishedSnapshot());
    REQUIRE(state.projectPath.empty());
    DirectorDesk::UI::AppViewState view;
    DirectorDesk::App::FrameStrings frame;
    DirectorDesk::App::BuildViewState(state, view, frame);
    REQUIRE(view.projectIsEmpty);

    DirectorDesk::Scene::Node cube;
    cube.id = "node-ph";
    cube.name = "立方体";
    state.scene.Add(cube);
    DirectorDesk::App::BuildViewState(state, view, frame);
    REQUIRE(view.projectIsEmpty);

    REQUIRE(state.script.LoadFromText("## [scene:s1] 场\n\n### [shot:a] 镜\n\n窗。\n").IsOk());
    DirectorDesk::App::BuildViewState(state, view, frame);
    REQUIRE_FALSE(view.projectIsEmpty);

    state.script.Reset();
    state.scene.Clear();
    DirectorDesk::Scene::Node imported;
    imported.id = "node-user";
    imported.name = "桌子";
    imported.sourcePath = "models/table.obj";
    state.scene.Add(imported);
    DirectorDesk::App::BuildViewState(state, view, frame);
    REQUIRE_FALSE(view.projectIsEmpty);

    state.scene.Clear();
    state.projectPath = "C:/tmp/cafe.ddproj";
    DirectorDesk::App::BuildViewState(state, view, frame);
    REQUIRE_FALSE(view.projectIsEmpty);
}

TEST_CASE("BuildViewState copies hasSkin onto NodeView", "[app][dispatch][fnd40]") {
    DirectorDesk::App::AppState state;
    DirectorDesk::Scene::Node node;
    node.id = "node-1";
    node.name = "角色";
    node.hasSkin = true;
    state.scene.Add(node);
    DirectorDesk::UI::AppViewState view;
    DirectorDesk::App::FrameStrings frame;
    DirectorDesk::App::BuildViewState(state, view, frame);
    REQUIRE(view.nodes != nullptr);
    REQUIRE(view.nodes->size() == 1);
    REQUIRE(view.nodes->front().hasSkin);
}

TEST_CASE("BuildViewState HUD focal matches Export formula", "[app][dispatch][uic21]") {
    DirectorDesk::App::AppState state;
    REQUIRE(
        state.script.LoadFromText("## [scene:s1] 场\n\n### [shot:shot-a] 过肩\n\n窗。\n").IsOk());
    state.script.SelectShot("shot-a");
    REQUIRE_FALSE(state.cameras.Cameras().empty());
    DirectorDesk::Camera::CameraRig* camera =
        state.cameras.Find(state.cameras.Cameras().front().id);
    REQUIRE(camera != nullptr);
    camera->orbit.SetFovYDegrees(45.0f);
    camera->name = "主镜头";
    state.links.Set("shot-a", camera->id);

    DirectorDesk::UI::AppViewState view;
    DirectorDesk::App::FrameStrings frame;
    DirectorDesk::App::BuildViewState(state, view, frame);
    REQUIRE(view.shotHud != nullptr);
    REQUIRE(std::string(view.shotHud->shotTitle) == "过肩");
    REQUIRE(std::string(view.shotHud->cameraName) == "主镜头");
    REQUIRE(view.shotHud->focalLength35mm ==
            Catch::Approx(DirectorDesk::Export::VerticalFovToFocalLength35mm(45.0f)).margin(0.01f));

    state.links.Clear();
    DirectorDesk::App::BuildViewState(state, view, frame);
    REQUIRE(view.shotHud != nullptr);
    REQUIRE(view.shotHud->focalLength35mm == 0.0f);
    REQUIRE(std::string(view.shotHud->cameraName) == "无机位");
}

TEST_CASE("Dispatch undo restores dressing and leaves script text", "[app][dispatch][fnd42]") {
    DirectorDesk::App::AppState state;
    REQUIRE(state.script.LoadFromText("## [scene:s1] 场\n### [shot:shot-a] 过肩\n窗。\n").IsOk());
    auto services = MakeServices();
    const std::string kept = "## [scene:s1] 场\n### [shot:shot-a] 过肩\n保留的剧本文本\n";
    DirectorDesk::App::Dispatch(state, DirectorDesk::Core::SetScriptTextCommand{kept}, services);
    REQUIRE(state.script.Text().find("保留的剧本文本") != std::string::npos);

    DirectorDesk::Scene::Node node;
    node.id = "node-1";
    node.name = "椅子";
    node.transform.position = {1.0f, 2.0f, 3.0f};
    state.scene.Add(node);
    DirectorDesk::Core::SetNodeTransformCommand transform;
    transform.nodeId = "node-1";
    transform.position[0] = 9.0f;
    transform.position[1] = 8.0f;
    transform.position[2] = 7.0f;
    DirectorDesk::App::Dispatch(state, transform, services);
    REQUIRE(state.scene.Find("node-1")->transform.position.x == 9.0f);
    REQUIRE(state.undoStack.size() == 1);

    DirectorDesk::App::Dispatch(state, DirectorDesk::Core::UndoCommand{}, services);
    REQUIRE(state.scene.Find("node-1")->transform.position.x == 1.0f);
    REQUIRE(state.script.Text().find("保留的剧本文本") != std::string::npos);
    REQUIRE(state.status == "已撤销");

    DirectorDesk::App::Dispatch(state, DirectorDesk::Core::RedoCommand{}, services);
    REQUIRE(state.scene.Find("node-1")->transform.position.x == 9.0f);
    REQUIRE(state.script.Text().find("保留的剧本文本") != std::string::npos);
}

TEST_CASE("Dispatch undo stack keeps twenty dressing steps", "[app][dispatch][fnd42]") {
    DirectorDesk::App::AppState state;
    auto services = MakeServices();
    for (int i = 0; i < 25; ++i) {
        DirectorDesk::App::Dispatch(state, DirectorDesk::Core::AddCameraCommand{}, services);
    }
    REQUIRE(state.undoStack.size() == 20);
}

TEST_CASE("Dispatch ImportModelFromPath installs SKILL.md", "[app][dispatch][skill]") {
    DirectorDesk::App::AppState state;
    const std::string dir = MakeLibDir("skill-install");
    REQUIRE(state.library.Open(dir).IsOk());
    const std::string skillPath = DirectorDesk::Platform::Paths::Join(dir, "SKILL.md");
    REQUIRE(DirectorDesk::Platform::Paths::WriteTextFile(skillPath, "# 分镜 Skill\n说明\n").IsOk());
    auto services = MakeServices();
    DirectorDesk::App::Dispatch(state, DirectorDesk::Core::ImportModelFromPathCommand{skillPath},
                                services);
    REQUIRE(state.status.find("已安装 Skill") != std::string::npos);
    REQUIRE_FALSE(state.library.Assets().empty());
    REQUIRE(state.library.Assets().front().format == "skill");
    REQUIRE(DirectorDesk::Platform::Paths::FileName(state.library.Assets().front().sourcePath) ==
            "SKILL.md");
    REQUIRE(state.library.Assets().front().sourcePath != skillPath);
    REQUIRE(state.libraryOriginFilter == "all");
    REQUIRE(state.scene.Nodes().empty());
}

TEST_CASE("BuildViewState exposes gizmo when a node is selected", "[app][dispatch][fnd41]") {
    DirectorDesk::App::AppState state;
    DirectorDesk::Scene::Node node;
    node.id = "node-1";
    node.name = "椅子";
    node.transform.position = {0.5f, 0.0f, 0.0f};
    state.scene.Add(node);
    state.scene.SetSelectedId("node-1");
    state.selectionKind = "node";
    state.selectionId = "node-1";
    DirectorDesk::UI::AppViewState view;
    DirectorDesk::App::FrameStrings frame;
    DirectorDesk::App::BuildViewState(state, view, frame);
    REQUIRE(view.gizmoActive);
    REQUIRE(view.exampleSkillPath != nullptr);
}

TEST_CASE("Dispatch mock image generation writes a local png", "[app][dispatch][ai]") {
    DirectorDesk::App::AppState state;
    state.userSettings.aiProvider = "mock";
    REQUIRE(state.script
                .LoadFromText("## [scene:s1] 场\n\n### [shot:shot-a] 过肩\n> 提示词: 暖光\n\n窗。\n")
                .IsOk());
    state.script.SelectShot("shot-a");
    auto services = MakeServices();
    DirectorDesk::App::Dispatch(state, DirectorDesk::Core::GenerateShotImageCommand{"shot-a"},
                                services);
    REQUIRE(state.status.find("已生成图像") != std::string::npos);
    REQUIRE(DirectorDesk::Platform::Paths::Exists(state.lastAiOutputPath));
    REQUIRE_FALSE(state.exportLog.empty());
    REQUIRE(state.exportLog.back().label == "ai-image");
    REQUIRE(state.exportLog.back().ok);
}

TEST_CASE("Dispatch openai-compat without key is rejected", "[app][dispatch][ai]") {
    DirectorDesk::App::AppState state;
    state.userSettings.aiProvider = "openai-compat";
    state.userSettings.aiApiKey.clear();
    REQUIRE(state.script
                .LoadFromText("## [scene:s1] 场\n\n### [shot:shot-a] 过肩\n\n窗。\n")
                .IsOk());
    state.script.SelectShot("shot-a");
    auto services = MakeServices();
    DirectorDesk::App::Dispatch(state, DirectorDesk::Core::GenerateShotImageCommand{}, services);
    REQUIRE(state.status == "请先填写 API 密钥");
    REQUIRE_FALSE(state.aiBusy);
}

TEST_CASE("Dispatch RunSkill imports bundled storyboard json", "[app][dispatch][ai][skill]") {
    DirectorDesk::App::AppState state;
    const std::string dir = MakeLibDir("skill-run");
    REQUIRE(state.library.Open(dir).IsOk());
    const std::string pack = DirectorDesk::Platform::Paths::Join(dir, "storyboard");
    REQUIRE(DirectorDesk::Platform::Paths::CreateDirectories(pack).IsOk());
    const std::string skillPath = DirectorDesk::Platform::Paths::Join(pack, "SKILL.md");
    REQUIRE(DirectorDesk::Platform::Paths::WriteTextFile(skillPath, "# Skill\n").IsOk());
    REQUIRE(DirectorDesk::Platform::Paths::WriteTextFile(
                DirectorDesk::Platform::Paths::Join(pack, "skill.json"),
                R"({"format":"DirectorDeskSkillRun","formatVersion":1,"output":"storyboard-import.json","builtin":"copy-output"})")
                .IsOk());
    REQUIRE(DirectorDesk::Platform::Paths::WriteTextFile(
                DirectorDesk::Platform::Paths::Join(pack, "storyboard-import.json"),
                R"({"format":"DirectorDeskStoryboardImport","formatVersion":1,"title":"Skill出","scenes":[{"id":"scene-run","title":"场","shots":[{"id":"shot-run","title":"镜","body":"A。"}]}]})")
                .IsOk());
    auto services = MakeServices();
    DirectorDesk::App::Dispatch(state, DirectorDesk::Core::ImportModelFromPathCommand{skillPath},
                                services);
    const std::string assetId = state.library.Assets().front().id;
    DirectorDesk::App::Dispatch(state, DirectorDesk::Core::RunSkillCommand{assetId}, services);
    REQUIRE(state.script.HasPublishedSnapshot());
    REQUIRE_FALSE(state.script.PublishedSnapshot().scenes.empty());
    REQUIRE(state.script.PublishedSnapshot().scenes.back().id.find("scene") != std::string::npos);
}

TEST_CASE("Dispatch RunSkill llm mock generates json from script", "[app][dispatch][ai][skill]") {
    DirectorDesk::App::AppState state;
    state.userSettings.aiProvider = "mock";
    REQUIRE(state.script.LoadFromText("## [scene:s1] 场\n\n### [shot:shot-a] 过肩\n\n窗。\n").IsOk());
    const std::string dir = MakeLibDir("skill-llm");
    REQUIRE(state.library.Open(dir).IsOk());
    const std::string pack = DirectorDesk::Platform::Paths::Join(dir, "storyboard");
    REQUIRE(DirectorDesk::Platform::Paths::CreateDirectories(pack).IsOk());
    const std::string skillPath = DirectorDesk::Platform::Paths::Join(pack, "SKILL.md");
    REQUIRE(DirectorDesk::Platform::Paths::WriteTextFile(skillPath, "# Skill\n").IsOk());
    REQUIRE(DirectorDesk::Platform::Paths::WriteTextFile(
                DirectorDesk::Platform::Paths::Join(pack, "skill.json"),
                R"({"format":"DirectorDeskSkillRun","formatVersion":1,"output":"storyboard-import.json","builtin":"openai-compat-json"})")
                .IsOk());
    auto services = MakeServices();
    DirectorDesk::App::Dispatch(state, DirectorDesk::Core::ImportModelFromPathCommand{skillPath},
                                services);
    const std::string assetId = state.library.Assets().front().id;
    DirectorDesk::App::Dispatch(state, DirectorDesk::Core::RunSkillCommand{assetId}, services);
    REQUIRE(state.status.find("已导入") != std::string::npos);
    REQUIRE(state.script.PublishedSnapshot().scenes.size() >= 2);
}

TEST_CASE("Dispatch RunSkill llm without key is rejected", "[app][dispatch][ai][skill]") {
    DirectorDesk::App::AppState state;
    state.userSettings.aiProvider = "openai-compat";
    state.userSettings.aiApiKey.clear();
    const std::string dir = MakeLibDir("skill-llm-key");
    REQUIRE(state.library.Open(dir).IsOk());
    const std::string pack = DirectorDesk::Platform::Paths::Join(dir, "storyboard");
    REQUIRE(DirectorDesk::Platform::Paths::CreateDirectories(pack).IsOk());
    const std::string skillPath = DirectorDesk::Platform::Paths::Join(pack, "SKILL.md");
    REQUIRE(DirectorDesk::Platform::Paths::WriteTextFile(skillPath, "# Skill\n").IsOk());
    REQUIRE(DirectorDesk::Platform::Paths::WriteTextFile(
                DirectorDesk::Platform::Paths::Join(pack, "skill.json"),
                R"({"format":"DirectorDeskSkillRun","formatVersion":1,"builtin":"openai-compat-json"})")
                .IsOk());
    auto services = MakeServices();
    DirectorDesk::App::Dispatch(state, DirectorDesk::Core::ImportModelFromPathCommand{skillPath},
                                services);
    DirectorDesk::App::Dispatch(state,
                                DirectorDesk::Core::RunSkillCommand{state.library.Assets().front().id},
                                services);
    REQUIRE(state.status == "请先填写 API 密钥");
    REQUIRE_FALSE(state.aiBusy);
}

TEST_CASE("BuildViewState fills scriptSelectedLine range for the selected shot",
          "[app][dispatch][uic22]") {
    DirectorDesk::App::AppState state;
    REQUIRE(state.script
                .LoadFromText("## [scene:s1] 场\n\n### [shot:shot-a] 过肩\n\n窗。\n")
                .IsOk());
    state.script.SelectShot("shot-a");
    DirectorDesk::UI::AppViewState view;
    DirectorDesk::App::FrameStrings frame;
    DirectorDesk::App::BuildViewState(state, view, frame);
    REQUIRE(view.scriptSelectedLineStart >= 3);
    REQUIRE(view.scriptSelectedLineEnd >= view.scriptSelectedLineStart);
}
