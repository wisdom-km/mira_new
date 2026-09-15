// ViewStateBuilder: Per-frame AppViewState assembly (FND-10).
#include "DirectorDesk/App/ViewStateBuilder.h"

#include "AppInternals.h"
#include "DirectorDesk/App/CommandDispatch.h"
#include "DirectorDesk/AI/SkillRunner.h"
#include "DirectorDesk/Asset/Manifest.h"
#include "DirectorDesk/Camera/Presets.h"
#include "DirectorDesk/Export/ShotExport.h"
#include "DirectorDesk/Platform/Paths.h"
#include "DirectorDesk/Scene/Document.h"
#include "DirectorDesk/Script/Parser.h"

#include <cstring>
#include <glm/gtc/type_ptr.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

namespace DirectorDesk::App {
namespace {

UI::SelectionKind SelectionKindFromId(const std::string& id) {
    if (id == "shot") {
        return UI::SelectionKind::Shot;
    }
    if (id == "node") {
        return UI::SelectionKind::Node;
    }
    if (id == "camera") {
        return UI::SelectionKind::Camera;
    }
    if (id == "asset") {
        return UI::SelectionKind::Asset;
    }
    return UI::SelectionKind::None;
}

std::string FirstLines(const std::string& utf8Path, int maxLines) {
    auto text = Platform::Paths::ReadTextFile(utf8Path);
    if (!text.IsOk()) {
        return {};
    }
    std::string excerpt;
    int lines = 0;
    for (char ch : text.Value()) {
        excerpt.push_back(ch);
        if (ch == '\n') {
            ++lines;
            if (lines >= maxLines) {
                break;
            }
        }
    }
    return excerpt;
}

bool IsSkillAsset(const std::string& format, const std::string& kind) {
    return format == "skill" || kind == "skill";
}

bool HasNonWhitespace(const std::string& text) {
    for (char ch : text) {
        if (ch != ' ' && ch != '\t' && ch != '\n' && ch != '\r') {
            return true;
        }
    }
    return false;
}

bool HasUserSceneNode(const AppState& state) {
    for (const Scene::Node& node : state.scene.Nodes()) {
        if (!node.sourcePath.empty() || !node.libraryAssetId.empty()) {
            return true;
        }
    }
    return false;
}

void FillSkillView(UI::LibraryAssetView& item, const Asset::OfficialCatalog& catalog,
                   const std::string& fallbackPath) {
    if (const Asset::ManifestAsset* official = catalog.FindAsset(item.id)) {
        item.kind = official->kind.empty() ? (official->format == "skill" ? "skill" : "model")
                                           : official->kind;
        if (item.format.empty()) {
            item.format = official->format;
        }
        item.hasSkin = official->kind == "character" ||
                       (!official->rigType.empty() && official->rigType != "none");
    } else if (item.kind.empty()) {
        item.kind = item.format == "skill" ? "skill" : "model";
    }
    const Asset::OfficialAssetState* catalogState = catalog.State(item.id);
    if (catalogState != nullptr && !catalogState->entrypointPath.empty()) {
        item.installPath = Platform::Paths::Parent(catalogState->entrypointPath);
        if (IsSkillAsset(item.format, item.kind)) {
            item.skillExcerpt = FirstLines(catalogState->entrypointPath, 20);
        }
    } else if (!fallbackPath.empty()) {
        item.installPath = Platform::Paths::Parent(fallbackPath);
        if (IsSkillAsset(item.format, item.kind)) {
            item.skillExcerpt = FirstLines(fallbackPath, 20);
        }
    }
    if (IsSkillAsset(item.format, item.kind)) {
        item.canAddToScene = false;
        if (!item.installPath.empty()) {
            item.canRunSkill =
                Platform::Paths::Exists(Platform::Paths::Join(item.installPath, "skill.json"));
            if (item.canRunSkill) {
                item.skillUsesLlm =
                    AI::SkillBuiltin(item.installPath) == "openai-compat-json";
            }
        }
    }
}

void FillShotHud(const AppState& state, UI::AppViewState& viewState, FrameStrings& frame) {
    frame.shotHud = {};
    frame.shotHudTitle.clear();
    frame.shotHudCamera.clear();
    if (!state.script.HasPublishedSnapshot()) {
        viewState.shotHud = nullptr;
        return;
    }
    const std::string& selectedId = state.script.SelectedShotId();
    if (selectedId.empty()) {
        viewState.shotHud = nullptr;
        return;
    }
    const Script::Shot* shot = nullptr;
    for (const Script::Scene& sceneItem : state.script.PublishedSnapshot().scenes) {
        for (const Script::Shot& item : sceneItem.shots) {
            if (item.id == selectedId) {
                shot = &item;
                break;
            }
        }
        if (shot != nullptr) {
            break;
        }
    }
    if (shot == nullptr) {
        viewState.shotHud = nullptr;
        return;
    }
    frame.shotHudTitle = shot->title;
    frame.shotHud.shotTitle = frame.shotHudTitle.c_str();
    if (const std::string* cameraId = state.links.CameraForShot(shot->id)) {
        if (const Camera::CameraRig* rig = state.cameras.Find(*cameraId)) {
            frame.shotHudCamera = rig->name;
            frame.shotHud.focalLength35mm =
                Export::VerticalFovToFocalLength35mm(rig->orbit.FovYDegrees());
        } else {
            frame.shotHudCamera = *cameraId;
            frame.shotHud.focalLength35mm = 0.0f;
        }
    } else {
        frame.shotHudCamera = "无机位";
        frame.shotHud.focalLength35mm = 0.0f;
    }
    frame.shotHud.cameraName = frame.shotHudCamera.c_str();
    viewState.shotHud = &frame.shotHud;
}

} // namespace

void BuildViewState(AppState& state, UI::AppViewState& viewState, FrameStrings& frame) {
    frame.nodes.clear();
    for (const Scene::Node& node : state.scene.Nodes()) {
        UI::NodeView item;
        item.id = node.id;
        item.name = node.name;
        item.position[0] = node.transform.position.x;
        item.position[1] = node.transform.position.y;
        item.position[2] = node.transform.position.z;
        const glm::vec3 euler = node.transform.EulerDegrees();
        item.eulerDegrees[0] = euler.x;
        item.eulerDegrees[1] = euler.y;
        item.eulerDegrees[2] = euler.z;
        item.scale[0] = node.transform.scale.x;
        item.scale[1] = node.transform.scale.y;
        item.scale[2] = node.transform.scale.z;
        item.selected = node.id == state.scene.SelectedId();
        item.visible = node.visible;
        item.hasSkin = node.hasSkin;
        frame.nodes.push_back(std::move(item));
    }

    viewState.statusText = state.status.c_str();
    viewState.importInProgress = state.importInProgress;
    viewState.sceneLoadPending = state.sceneLoadPending;
    viewState.sceneLoadTotal = state.sceneLoadTotal;
    viewState.projectSaveInProgress = state.projectSaveInProgress;
    viewState.nodes = &frame.nodes;
    viewState.exampleObjPath =
        Platform::Paths::Exists(state.exampleObj) ? state.exampleObj.c_str() : "";
    viewState.exampleGlbPath =
        Platform::Paths::Exists(state.exampleGlb) ? state.exampleGlb.c_str() : "";
    viewState.exampleScriptPath =
        Platform::Paths::Exists(state.exampleScript) ? state.exampleScript.c_str() : "";
    viewState.exampleStoryboardImportPath = Platform::Paths::Exists(state.exampleStoryboardImport)
                                                ? state.exampleStoryboardImport.c_str()
                                                : "";
    viewState.exampleProjectPath =
        Platform::Paths::Exists(state.exampleProject) ? state.exampleProject.c_str() : "";
    viewState.exampleSkillPath =
        Platform::Paths::Exists(state.exampleSkill) ? state.exampleSkill.c_str() : "";

    frame.scriptScenes.clear();
    if (state.script.HasPublishedSnapshot()) {
        for (const Script::Scene& sceneItem : state.script.PublishedSnapshot().scenes) {
            UI::ScriptSceneView sceneView;
            sceneView.id = sceneItem.id;
            sceneView.title = sceneItem.title;
            for (const Script::Shot& shot : sceneItem.shots) {
                UI::ScriptShotView shotView;
                shotView.id = shot.id;
                shotView.title = shot.title;
                shotView.selected = shot.id == state.script.SelectedShotId();
                if (const std::string* cameraId = state.links.CameraForShot(shot.id)) {
                    shotView.linkedCameraId = *cameraId;
                    if (const Camera::CameraRig* rig = state.cameras.Find(*cameraId)) {
                        shotView.linkedCameraName = rig->name;
                    } else {
                        shotView.linkedCameraName = *cameraId;
                        shotView.linkedMissing = true;
                    }
                }
                sceneView.shots.push_back(std::move(shotView));
            }
            frame.scriptScenes.push_back(std::move(sceneView));
        }
    }
    frame.scriptDiagnostics.clear();
    for (const Script::Diagnostic& diagnostic : state.script.Diagnostics()) {
        UI::ScriptDiagnosticView item;
        item.severity = DiagnosticSeverityText(diagnostic.severity);
        item.line = diagnostic.line;
        item.code = diagnostic.code.c_str();
        item.message = diagnostic.message.c_str();
        frame.scriptDiagnostics.push_back(item);
    }

    viewState.scriptText = state.script.Text().c_str();
    viewState.scriptPath = state.script.Path().c_str();
    viewState.scriptDirty = state.script.IsDirty();
    viewState.scriptHasSnapshot = state.script.HasPublishedSnapshot();
    viewState.scriptExternalRevision = state.script.ExternalRevision();
    viewState.scriptScenes = &frame.scriptScenes;
    viewState.scriptSelectedLineStart = 0;
    viewState.scriptSelectedLineEnd = 0;
    const std::string& selectedShotId = state.script.SelectedShotId();
    if (!selectedShotId.empty() && state.script.HasPublishedSnapshot()) {
        for (const Script::Scene& sceneItem : state.script.PublishedSnapshot().scenes) {
            for (const Script::Shot& shot : sceneItem.shots) {
                if (shot.id == selectedShotId) {
                    viewState.scriptSelectedLineStart = shot.lineStart;
                    viewState.scriptSelectedLineEnd = shot.lineEnd;
                    break;
                }
            }
        }
    }
    viewState.scriptDiagnostics = &frame.scriptDiagnostics;

    frame.cameras.clear();
    for (const Camera::CameraRig& rig : state.cameras.Cameras()) {
        UI::CameraItemView item;
        item.id = rig.id;
        item.name = rig.name;
        item.selected = rig.id == state.cameras.SelectedId();
        frame.cameras.push_back(std::move(item));
    }
    viewState.cameras = &frame.cameras;
    viewState.lightPresetId = Camera::LightPresetId(state.cameras.LightPreset());
    if (const Camera::CameraRig* rig = state.cameras.Selected()) {
        const glm::vec3 position = rig->orbit.Position();
        viewState.selectedCameraPosition[0] = position.x;
        viewState.selectedCameraPosition[1] = position.y;
        viewState.selectedCameraPosition[2] = position.z;
    }

    frame.libraryAssets.clear();
    frame.officialCategories.clear();
    frame.libraryPreviewPaths.clear();
    if (state.libraryOriginFilter == "online") {
        for (const Asset::ManifestCategory& category :
             state.officialCatalog.Manifest().categories) {
            frame.officialCategories.push_back(category.id);
        }
        for (const Asset::ManifestAsset& asset :
             state.officialCatalog.Query(state.librarySearch, state.officialCategory)) {
            UI::LibraryAssetView item;
            item.id = asset.id;
            item.name = Asset::PickLocale(asset.name);
            item.format = asset.format;
            item.origin = "online";
            item.category = asset.category;
            item.description = Asset::PickLocale(asset.description);
            item.license = asset.license.spdx;
            if (!asset.license.attribution.empty()) {
                item.license += " · " + asset.license.attribution;
            }
            item.author = asset.author.name;
            const Asset::OfficialAssetState* catalogState = state.officialCatalog.State(asset.id);
            const Asset::OfficialDownloadStatus downloadStatus =
                catalogState == nullptr ? Asset::OfficialDownloadStatus::NotDownloaded
                                        : catalogState->status;
            item.status = Asset::OfficialCatalog::StatusId(downloadStatus);
            item.progress = catalogState == nullptr ? 0.0f : catalogState->progress;
            item.canDownload = downloadStatus == Asset::OfficialDownloadStatus::NotDownloaded ||
                               downloadStatus == Asset::OfficialDownloadStatus::Failed ||
                               downloadStatus == Asset::OfficialDownloadStatus::Cancelled;
            item.canCancel = downloadStatus == Asset::OfficialDownloadStatus::Queued ||
                             downloadStatus == Asset::OfficialDownloadStatus::Downloading;
            item.canAddToScene = downloadStatus == Asset::OfficialDownloadStatus::Ready;
            item.missing = downloadStatus != Asset::OfficialDownloadStatus::Ready;
            item.selected = asset.id == state.selectedLibraryAssetId;
            if (downloadStatus == Asset::OfficialDownloadStatus::Failed &&
                catalogState != nullptr && !catalogState->message.empty()) {
                item.status = catalogState->message;
            }
            if (catalogState != nullptr && !catalogState->previewPath.empty()) {
                frame.libraryPreviewPaths[asset.id] = catalogState->previewPath;
            }
            FillSkillView(item, state.officialCatalog,
                          catalogState != nullptr ? catalogState->entrypointPath : std::string{});
            frame.libraryAssets.push_back(std::move(item));
        }
    } else {
        for (const Asset::LibraryAsset& asset :
             state.library.Query(state.librarySearch, state.libraryOriginFilter)) {
            UI::LibraryAssetView item;
            item.id = asset.id;
            item.name = asset.name;
            item.format = asset.format;
            item.origin = Asset::Library::OriginId(asset.origin);
            item.category = asset.category;
            item.missing = !asset.sourceExists;
            item.status = asset.sourceExists ? "就绪" : "缺失";
            item.canAddToScene = asset.sourceExists;
            item.selected = asset.id == state.selectedLibraryAssetId;
            if (!asset.previewPath.empty()) {
                frame.libraryPreviewPaths[asset.id] = asset.previewPath;
            }
            FillSkillView(item, state.officialCatalog, asset.sourcePath);
            frame.libraryAssets.push_back(std::move(item));
        }
    }
    auto hasLibraryId = [&](const std::string& id) {
        for (const UI::LibraryAssetView& item : frame.libraryAssets) {
            if (item.id == id) {
                return true;
            }
        }
        return false;
    };
    for (const Asset::LibraryAsset& asset : state.library.Query("", "all")) {
        if (asset.format != "skill" || hasLibraryId(asset.id)) {
            continue;
        }
        UI::LibraryAssetView item;
        item.id = asset.id;
        item.name = asset.name;
        item.format = asset.format;
        item.origin = Asset::Library::OriginId(asset.origin);
        item.category = asset.category;
        item.missing = !asset.sourceExists;
        item.status = asset.sourceExists ? "就绪" : "缺失";
        item.selected = asset.id == state.selectedLibraryAssetId;
        FillSkillView(item, state.officialCatalog, asset.sourcePath);
        frame.libraryAssets.push_back(std::move(item));
    }
    if (state.officialCatalog.IsConfigured()) {
        for (const Asset::ManifestAsset& asset : state.officialCatalog.Query("", "")) {
            if (!IsSkillAsset(asset.format, asset.kind) || hasLibraryId(asset.id)) {
                continue;
            }
            UI::LibraryAssetView item;
            item.id = asset.id;
            item.name = Asset::PickLocale(asset.name);
            item.format = asset.format;
            item.origin = "online";
            item.category = asset.category;
            item.description = Asset::PickLocale(asset.description);
            const Asset::OfficialAssetState* catalogState = state.officialCatalog.State(asset.id);
            const Asset::OfficialDownloadStatus downloadStatus =
                catalogState == nullptr ? Asset::OfficialDownloadStatus::NotDownloaded
                                        : catalogState->status;
            item.status = Asset::OfficialCatalog::StatusId(downloadStatus);
            item.progress = catalogState == nullptr ? 0.0f : catalogState->progress;
            item.canDownload = downloadStatus == Asset::OfficialDownloadStatus::NotDownloaded ||
                               downloadStatus == Asset::OfficialDownloadStatus::Failed ||
                               downloadStatus == Asset::OfficialDownloadStatus::Cancelled;
            item.canCancel = downloadStatus == Asset::OfficialDownloadStatus::Queued ||
                             downloadStatus == Asset::OfficialDownloadStatus::Downloading;
            item.missing = downloadStatus != Asset::OfficialDownloadStatus::Ready;
            FillSkillView(item, state.officialCatalog,
                          catalogState != nullptr ? catalogState->entrypointPath : std::string{});
            frame.libraryAssets.push_back(std::move(item));
        }
    }
    viewState.libraryAssets = &frame.libraryAssets;
    viewState.librarySearch = state.librarySearch.c_str();
    viewState.libraryOriginFilter = state.libraryOriginFilter.c_str();
    viewState.libraryViewMode = state.libraryViewMode.c_str();
    viewState.officialCategory = state.officialCategory.c_str();
    viewState.officialCatalogStatus = state.officialCatalogStatus.c_str();
    viewState.officialConfigured = state.officialCatalog.IsConfigured();
    viewState.officialCategories = &frame.officialCategories;

    RefreshBoard(state);

    frame.storyboardCards.clear();
    for (const Storyboard::LayoutCard& card : state.storyboard.Layout().cards) {
        UI::StoryboardCardView item;
        item.id = card.id;
        item.title = card.title;
        item.shotId = card.shotId;
        item.sceneId = card.sceneId;
        item.x = card.x;
        item.y = card.y;
        item.w = card.w;
        item.h = card.h;
        item.selected = card.selected;
        item.collapsed = card.collapsed;
        item.kind = card.kind == Storyboard::CardKind::Root
                        ? "root"
                        : (card.kind == Storyboard::CardKind::Scene ? "scene" : "shot");
        item.kindEnum = card.kind == Storyboard::CardKind::Root
                            ? UI::CardKind::Root
                            : (card.kind == Storyboard::CardKind::Scene ? UI::CardKind::Scene
                                                                        : UI::CardKind::Shot);
        item.linked = card.link == Storyboard::LinkStatus::Linked;
        item.linkEnum = item.linked ? UI::LinkStatus::Linked : UI::LinkStatus::Unlinked;
        item.link = item.linked ? "已关联" : "未关联";
        item.preview = PreviewText(card.preview);
        item.previewEnum = static_cast<UI::PreviewStatus>(card.preview);
        item.exported = card.exported == Storyboard::ExportStatus::Exported ? "已导出" : "未导出";
        item.exportEnum = card.exported == Storyboard::ExportStatus::Exported
                              ? UI::ExportStatus::Exported
                              : UI::ExportStatus::NotExported;
        item.metaLine = card.metaLine;
        if (const Storyboard::ThumbnailRecord* thumb = state.storyboard.Thumbnail(card.shotId)) {
            item.thumbTexture = thumb->textureIndex;
        }
        frame.storyboardCards.push_back(std::move(item));
    }
    viewState.storyboardCards = &frame.storyboardCards;
    viewState.storyboardContentWidth = state.storyboard.Layout().contentWidth;
    viewState.storyboardContentHeight = state.storyboard.Layout().contentHeight;
    viewState.storyboardHeldLastValid = state.storyboard.HeldLastValid();
    viewState.exportTransparent = state.exportTransparent;
    viewState.exportOverwritePrompt = state.exportOverwritePrompt;
    viewState.exportStalePrompt = state.exportStalePrompt;
    viewState.exportStaleCount = state.exportStaleCount;
    viewState.exportPendingPath = state.exportPendingPath.c_str();
    viewState.projectName = state.projectName.c_str();
    viewState.projectPath = state.projectPath.c_str();
    viewState.projectDirty = state.projectDirty;
    viewState.projectIsEmpty = state.projectPath.empty() &&
                               !HasNonWhitespace(state.script.Text()) && !HasUserSceneNode(state);
    viewState.projectPromptVisible = state.pendingAction != PendingProjectAction::None;
    if (const std::string* cameraId = state.links.CameraForShot(state.script.SelectedShotId())) {
        if (const Camera::CameraRig* rig = state.cameras.Find(*cameraId)) {
            frame.selectedShotLinkedCamera = rig->name;
        } else {
            frame.selectedShotLinkedCamera = *cameraId;
        }
        viewState.selectedShotLinkedCamera = frame.selectedShotLinkedCamera.c_str();
    } else {
        frame.selectedShotLinkedCamera.clear();
        viewState.selectedShotLinkedCamera = "";
    }

    if (state.selectionKind == "shot") {
        state.selectionLabel = FindShotTitle(state.script, state.selectionId);
        if (state.selectionLabel.empty()) {
            state.selectionKind = "none";
            state.selectionId.clear();
        }
    } else if (state.selectionKind == "node") {
        state.selectionLabel.clear();
        if (const Scene::Node* node = state.scene.Find(state.selectionId)) {
            state.selectionLabel = node->name;
        } else {
            state.selectionKind = "none";
            state.selectionId.clear();
        }
    } else if (state.selectionKind == "camera") {
        state.selectionLabel.clear();
        if (const Camera::CameraRig* rig = state.cameras.Find(state.selectionId)) {
            state.selectionLabel = rig->name;
        } else {
            state.selectionKind = "none";
            state.selectionId.clear();
        }
    } else if (state.selectionKind == "asset") {
        state.selectionLabel =
            LibraryAssetLabel(state.library, state.officialCatalog, state.selectionId);
        const bool inLibrary = state.library.Find(state.selectionId) != nullptr;
        const bool inOfficial = state.officialCatalog.FindAsset(state.selectionId) != nullptr;
        if (!inLibrary && !inOfficial) {
            state.selectionKind = "none";
            state.selectionId.clear();
            state.selectionLabel.clear();
            state.selectedLibraryAssetId.clear();
        }
    }
    if (state.selectionKind == "none") {
        state.selectionLabel.clear();
    }

    frame.exportIssues.clear();
    for (const UI::StoryboardCardView& card : frame.storyboardCards) {
        if (card.kindEnum != UI::CardKind::Shot) {
            continue;
        }
        const char* reason = nullptr;
        if (!card.linked) {
            reason = "unlinked-camera";
        } else if (card.previewEnum == UI::PreviewStatus::Stale) {
            reason = "preview-stale";
        } else if (card.previewEnum == UI::PreviewStatus::Failed) {
            reason = "preview-failed";
        } else if (card.previewEnum == UI::PreviewStatus::Missing) {
            reason = "preview-missing";
        }
        if (reason == nullptr) {
            continue;
        }
        UI::ExportIssueView issue;
        issue.shotId = card.shotId;
        issue.shotTitle = card.title;
        issue.reason = reason;
        frame.exportIssues.push_back(std::move(issue));
    }

    viewState.workspaceModeId = state.workspaceModeId.c_str();
    viewState.layoutRebuildRequested = state.layoutRebuildRequested;
    viewState.selectionKindEnum = SelectionKindFromId(state.selectionKind);
    viewState.selectionKind = state.selectionKind.c_str();
    viewState.selectionId = state.selectionId.c_str();
    viewState.selectionLabel = state.selectionLabel.c_str();
    viewState.exportIssues = &frame.exportIssues;
    viewState.exportLog = &state.exportLog;
    viewState.exportResolutionId = state.exportResolutionId.c_str();

    frame.selectedShotMeta.clear();
    if (state.script.HasPublishedSnapshot()) {
        for (const Script::Scene& sceneItem : state.script.PublishedSnapshot().scenes) {
            for (const Script::Shot& shot : sceneItem.shots) {
                if (shot.id != state.script.SelectedShotId()) {
                    continue;
                }
                for (const Script::ShotMeta& meta : shot.meta) {
                    frame.selectedShotMeta.push_back(UI::ShotMetaView{meta.key, meta.value});
                }
            }
        }
    }
    viewState.selectedShotMeta = &frame.selectedShotMeta;
    frame.importDiagnostics = state.importDiagnostics;
    viewState.importDiagnostics = &frame.importDiagnostics;
    FillShotHud(state, viewState, frame);
    viewState.canUndo = !state.undoStack.empty();
    viewState.canRedo = !state.redoStack.empty();
    viewState.gizmoActive = false;
    const float aspect = state.viewportHeight == 0
                             ? 1.0f
                             : static_cast<float>(state.viewportWidth) /
                                   static_cast<float>(state.viewportHeight);
    if (const Camera::CameraRig* camera = state.cameras.Selected()) {
        const Renderer::CameraView view = camera->orbit.BuildView(aspect);
        std::memcpy(viewState.gizmoView, glm::value_ptr(view.view), sizeof(viewState.gizmoView));
        std::memcpy(viewState.gizmoProj, glm::value_ptr(view.projection),
                    sizeof(viewState.gizmoProj));
    }
    if (state.selectionKind == "node") {
        if (const Scene::Node* node = state.scene.Find(state.selectionId)) {
            viewState.gizmoActive = true;
            const glm::mat4 world = node->transform.ToMatrix();
            std::memcpy(viewState.gizmoWorld, glm::value_ptr(world), sizeof(viewState.gizmoWorld));
        }
    }
    viewState.aiProvider = state.userSettings.aiProvider.c_str();
    viewState.aiBaseUrl = state.userSettings.aiBaseUrl.c_str();
    viewState.aiImageModel = state.userSettings.aiImageModel.c_str();
    viewState.aiVideoModel = state.userSettings.aiVideoModel.c_str();
    viewState.aiChatModel = state.userSettings.aiChatModel.c_str();
    viewState.aiHasApiKey = !state.userSettings.aiApiKey.empty();
    viewState.aiBusy = state.aiBusy;
    viewState.aiJobStatus = state.aiJobStatus.c_str();
    viewState.aiJobMessage = state.aiJobMessage.c_str();
    viewState.aiJobRatio = state.aiJobRatio;
    viewState.lastAiOutputPath = state.lastAiOutputPath.c_str();
    viewState.canGenerateAi =
        state.userSettings.aiProvider == "mock" || !state.userSettings.aiApiKey.empty();
    viewState.canRunSelectedSkill = false;
    if (state.selectionKind == "asset" && viewState.libraryAssets != nullptr) {
        for (const UI::LibraryAssetView& asset : *viewState.libraryAssets) {
            if (asset.selected && asset.canRunSkill) {
                viewState.canRunSelectedSkill = true;
                break;
            }
        }
    }
}

} // namespace DirectorDesk::App
