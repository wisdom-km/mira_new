// AppState: Cross-frame App state extracted from Application::Run (FND-10 / CR-10).
#pragma once

#include "DirectorDesk/App/UserSettings.h"
#include "DirectorDesk/Asset/Library.h"
#include "DirectorDesk/Asset/LoaderRegistry.h"
#include "DirectorDesk/Asset/ModelLoadResult.h"
#include "DirectorDesk/Asset/OfficialCatalog.h"
#include "DirectorDesk/Camera/CameraManager.h"
#include "DirectorDesk/Core/ResultQueue.h"
#include "DirectorDesk/Export/ShotExport.h"
#include "DirectorDesk/Link/ShotLink.h"
#include "DirectorDesk/Scene/Document.h"
#include "DirectorDesk/Script/Document.h"
#include "DirectorDesk/Storyboard/Document.h"
#include "DirectorDesk/UI/IPanel.h"

#include <atomic>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace DirectorDesk::App {

enum class PendingProjectAction {
    None,
    Quit,
    New,
    Open,
    ImportStoryboard,
};

struct LibraryPreviewGpu {
    std::uint16_t texture = 0xFFFFu;
    std::string path;
};

struct OfficialRefreshResult {
    bool ok = false;
    int status = 0;
    std::string body;
    std::string message;
};

struct OfficialDownloadJobResult {
    std::string assetId;
    bool ok = false;
    Asset::OfficialAssetState state;
    std::string message;
    Asset::ManifestAsset asset;
};

struct OfficialProgressUpdate {
    std::string assetId;
    float progress = 0.0f;
};

struct FileHashResult {
    std::string path;
    bool ok = false;
    std::string sha256;
    std::string message;
};

struct SaveHashJobResult {
    std::uint64_t generation = 0;
    std::string savePath;
    std::vector<FileHashResult> hashes;
};

struct DressingSnapshot {
    std::vector<Scene::Node> nodes;
    std::string selectedNodeId;
    std::vector<Camera::CameraRig> cameras;
    std::string selectedCameraId;
    Camera::LightPresetKind light = Camera::LightPresetKind::Neutral;
    std::vector<Link::ShotLink> links;
    std::string selectionKind = "none";
    std::string selectionId;
    std::string selectionLabel;
};

struct AiJobResult {
    std::string jobId;
    std::string kind;
    bool ok = false;
    std::string outputPath;
    std::string message;
    std::string shotId;
    std::string importJsonPath;
};

struct AppState {
    AppState();
    explicit AppState(std::string officialCacheRoot);

    Camera::CameraManager cameras;
    Scene::Document scene;
    Script::Document script;
    Link::Table links;
    Storyboard::Document storyboard;
    Storyboard::ThumbnailScheduler thumbScheduler;
    Asset::Library library;
    Asset::LoaderRegistry registry;
    Asset::OfficialCatalog officialCatalog;

    std::string officialCache;
    std::string status;
    std::string librarySearch;
    std::string libraryOriginFilter = "all";
    std::string libraryViewMode = "grid";
    std::string selectedLibraryAssetId;
    std::string officialCategory;
    std::string officialCatalogStatus;
    bool officialRefreshInFlight = false;
    std::string projectId;
    std::string projectName = "未命名工程";
    std::string projectPath;
    std::vector<std::string> collapsedScenes;
    std::string storyboardLayout = "grid";
    std::string pendingOpenPath;
    PendingProjectAction pendingAction = PendingProjectAction::None;
    bool projectDirty = false;
    bool importInProgress = false;
    bool projectSaveInProgress = false;
    bool projectSaveQueued = false;
    bool proceedAfterSave = false;
    std::string projectSavePendingPath;
    std::uint64_t projectGeneration = 0;
    std::uint32_t sceneLoadPending = 0;
    std::uint32_t sceneLoadTotal = 0;
    std::optional<Asset::ModelLoadResult> heldLoadResult;
    bool exportTransparent = true;
    bool exportOverwritePrompt = false;
    bool exportStalePrompt = false;
    int exportStaleCount = 0;
    std::string exportPendingPath;
    bool exportPendingBoard = false;
    bool exportPendingPackage = false;
    std::string exportPendingShotId;
    Export::ShotResolution exportPendingResolution = Export::ShotResolution::Hd1080;
    std::string exportResolutionId = "1080p";
    float storyboardViewPanX = 32.0f;
    float storyboardViewPanY = 32.0f;
    float storyboardViewZoom = 1.0f;
    float storyboardViewWidth = 0.0f;
    float storyboardViewHeight = 0.0f;
    std::string pendingThumbShotId;
    std::uint64_t frameIndex = 1;
    std::string workspaceModeId = "shoot";
    bool layoutRebuildRequested = false;
    std::string selectionKind = "none";
    std::string selectionId;
    std::string selectionLabel;
    std::vector<UI::ExportLogView> exportLog;
    std::unordered_map<std::string, LibraryPreviewGpu> libraryPreviewGpu;
    std::unordered_map<std::string, std::string> libraryPreviewFailed;
    std::unordered_map<std::string, std::shared_ptr<std::atomic<bool>>> officialCancels;
    std::unordered_map<std::uint32_t, std::uint32_t> gpuModelRefs;
    std::string exampleObj;
    std::string exampleGlb;
    std::string exampleScript;
    std::string exampleProject;
    std::string exampleStoryboardImport;
    std::string exampleSkill;
    std::vector<std::string> importDiagnostics;
    std::string pendingImportPath;
    std::string pendingImportMode;
    bool exportPendingPdf = false;
    std::uint32_t viewportWidth = 1280;
    std::uint32_t viewportHeight = 720;
    std::vector<DressingSnapshot> undoStack;
    std::vector<DressingSnapshot> redoStack;
    std::string undoCoalesceKey;
    UserSettings userSettings;
    std::string userSettingsPath;
    std::string aiJobId;
    std::string aiJobKind;
    std::string aiJobShotId;
    std::string aiJobStatus;
    std::string aiJobMessage;
    float aiJobRatio = 0.0f;
    bool aiBusy = false;
    std::string lastAiOutputPath;
    std::shared_ptr<std::atomic<bool>> aiCancel;
};

} // namespace DirectorDesk::App
