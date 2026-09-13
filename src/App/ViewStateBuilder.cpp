// ViewStateBuilder: Per-frame AppViewState assembly (FND-10).
#include "DirectorDesk/App/ViewStateBuilder.h"

#include "AppInternals.h"
#include "DirectorDesk/App/CommandDispatch.h"
#include "DirectorDesk/Asset/Manifest.h"
#include "DirectorDesk/Camera/Presets.h"
#include "DirectorDesk/Platform/Paths.h"
#include "DirectorDesk/Scene/Document.h"

#include <cstring>
#include <glm/vec3.hpp>

#include <cstring>
#include <glm/vec3.hpp>

namespace DirectorDesk::App {

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
    viewState.exampleProjectPath =
        Platform::Paths::Exists(state.exampleProject) ? state.exampleProject.c_str() : "";

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
        for (const Asset::ManifestCategory& category : state.officialCatalog.Manifest().categories) {
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
            if (downloadStatus == Asset::OfficialDownloadStatus::Failed && catalogState != nullptr &&
                !catalogState->message.empty()) {
                item.status = catalogState->message;
            }
            if (catalogState != nullptr && !catalogState->previewPath.empty()) {
                frame.libraryPreviewPaths[asset.id] = catalogState->previewPath;
            }
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
        item.link = card.link == Storyboard::LinkStatus::Linked ? "已关联" : "未关联";
        item.preview = PreviewText(card.preview);
        item.exported =
            card.exported == Storyboard::ExportStatus::Exported ? "已导出" : "未导出";
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
        if (card.kind != "shot") {
            continue;
        }
        const char* reason = nullptr;
        if (card.link != nullptr && std::strcmp(card.link, "未关联") == 0) {
            reason = "未关联相机";
        } else if (card.preview != nullptr && std::strcmp(card.preview, "过期") == 0) {
            reason = "缩略图过期";
        } else if (card.preview != nullptr && std::strcmp(card.preview, "失败") == 0) {
            reason = "渲染失败";
        } else if (card.preview != nullptr && std::strcmp(card.preview, "缺失") == 0) {
            reason = "缺少缩略图";
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
    viewState.selectionKind = state.selectionKind.c_str();
    viewState.selectionId = state.selectionId.c_str();
    viewState.selectionLabel = state.selectionLabel.c_str();
    viewState.exportIssues = &frame.exportIssues;
    viewState.exportLog = &state.exportLog;
    viewState.exportResolutionId = state.exportResolutionId.c_str();
}

} // namespace DirectorDesk::App
