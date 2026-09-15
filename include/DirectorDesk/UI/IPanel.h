// IPanel: Public or internal interface for the DirectorDesk UI module.
// This file owns project behavior only; keep platform and dependency boundaries explicit.

#pragma once

#include "DirectorDesk/Core/CommandQueue.h"

#include <cstdint>
#include <string>
#include <vector>

namespace DirectorDesk::UI {

enum class SelectionKind : std::uint8_t { None, Shot, Node, Camera, Asset };
enum class CardKind : std::uint8_t { Root, Scene, Shot };
enum class LinkStatus : std::uint8_t { Unlinked, Linked };
enum class PreviewStatus : std::uint8_t { Missing, Stale, Rendering, Ready, Failed };
enum class ExportStatus : std::uint8_t { NotExported, Exported };

struct NodeView {
    std::string id;
    std::string name;
    float position[3] = {0.0f, 0.0f, 0.0f};
    float eulerDegrees[3] = {0.0f, 0.0f, 0.0f};
    float scale[3] = {1.0f, 1.0f, 1.0f};
    bool selected = false;
    bool visible = true;
    bool hasSkin = false;
};

struct ScriptShotView {
    std::string id;
    std::string title;
    std::string linkedCameraId;
    std::string linkedCameraName;
    bool linkedMissing = false;
    bool selected = false;
};

struct ShotMetaView {
    std::string key;
    std::string value;
};

struct ScriptSceneView {
    std::string id;
    std::string title;
    std::vector<ScriptShotView> shots;
};

struct ScriptDiagnosticView {
    const char* severity = "";
    int line = 1;
    const char* code = "";
    const char* message = "";
};

struct CameraItemView {
    std::string id;
    std::string name;
    bool selected = false;
};

struct LibraryAssetView {
    std::string id;
    std::string name;
    std::string format;
    std::string origin;
    std::string status;
    std::string category;
    std::string description;
    std::string license;
    std::string author;
    float progress = 0.0f;
    bool missing = false;
    bool selected = false;
    bool canDownload = false;
    bool canCancel = false;
    std::string kind;
    std::string installPath;
    std::string skillExcerpt;
    bool canAddToScene = false;
    bool hasSkin = false;
    bool canRunSkill = false;
    bool skillUsesLlm = false;
    std::uint16_t previewTexture = 0xFFFFu;
};

struct ShotHudView {
    const char* shotTitle = "";
    const char* cameraName = "";
    float focalLength35mm = 0.0f;
};

struct StoryboardCardView {
    std::string id;
    std::string title;
    std::string kind;
    CardKind kindEnum = CardKind::Shot;
    std::string shotId;
    std::string sceneId;
    float x = 0.0f;
    float y = 0.0f;
    float w = 0.0f;
    float h = 0.0f;
    bool selected = false;
    bool collapsed = false;
    bool linked = false;
    LinkStatus linkEnum = LinkStatus::Unlinked;
    const char* link = "";
    const char* preview = "";
    PreviewStatus previewEnum = PreviewStatus::Missing;
    const char* exported = "";
    ExportStatus exportEnum = ExportStatus::NotExported;
    std::string metaLine;
    std::uint16_t thumbTexture = 0xFFFFu;
};

struct ExportIssueView {
    std::string shotId;
    std::string shotTitle;
    const char* reason = "";
};

struct ExportLogView {
    std::string label;
    std::string shotId;
    std::string shotTitle;
    std::string path;
    bool ok = false;
    std::string message;
};

struct AppViewState {
    // UI panels receive this immutable snapshot and communicate back only through commands.
    const char* appName = "DirectorDesk";
    unsigned windowWidth = 0;
    unsigned windowHeight = 0;
    std::uint16_t viewportTextureIndex = 0xFFFFu;
    unsigned viewportTextureWidth = 0;
    unsigned viewportTextureHeight = 0;
    const char* statusText = "";
    bool importInProgress = false;
    std::uint32_t sceneLoadPending = 0;
    std::uint32_t sceneLoadTotal = 0;
    bool projectSaveInProgress = false;
    const std::vector<NodeView>* nodes = nullptr;
    const char* exampleObjPath = "";
    const char* exampleGlbPath = "";
    const char* exampleScriptPath = "";
    const char* exampleStoryboardImportPath = "";
    const char* exampleProjectPath = "";
    const char* scriptText = "";
    const char* scriptPath = "";
    bool scriptDirty = false;
    bool scriptHasSnapshot = false;
    std::uint64_t scriptExternalRevision = 0;
    const std::vector<ScriptSceneView>* scriptScenes = nullptr;
    const std::vector<ScriptDiagnosticView>* scriptDiagnostics = nullptr;
    const std::vector<CameraItemView>* cameras = nullptr;
    const char* lightPresetId = "neutral";
    const std::vector<LibraryAssetView>* libraryAssets = nullptr;
    const char* librarySearch = "";
    const char* libraryOriginFilter = "all";
    const char* libraryViewMode = "grid";
    const char* officialCategory = "";
    const char* officialCatalogStatus = "";
    bool officialConfigured = false;
    const std::vector<std::string>* officialCategories = nullptr;
    const char* projectName = "";
    const char* projectPath = "";
    bool projectDirty = false;
    bool projectPromptVisible = false;
    const char* selectedShotLinkedCamera = "";
    const std::vector<StoryboardCardView>* storyboardCards = nullptr;
    float storyboardContentWidth = 0.0f;
    float storyboardContentHeight = 0.0f;
    float storyboardPanX = 0.0f;
    float storyboardPanY = 0.0f;
    float storyboardZoom = 1.0f;
    bool storyboardHeldLastValid = false;
    bool exportTransparent = true;
    bool exportOverwritePrompt = false;
    bool exportStalePrompt = false;
    int exportStaleCount = 0;
    const char* exportPendingPath = "";
    const char* workspaceModeId = "shoot";
    bool layoutRebuildRequested = false;
    SelectionKind selectionKindEnum = SelectionKind::None;
    const char* selectionKind = "none";
    const char* selectionId = "";
    const char* selectionLabel = "";
    const std::vector<ExportIssueView>* exportIssues = nullptr;
    const std::vector<ExportLogView>* exportLog = nullptr;
    const char* exportResolutionId = "1080p";
    float selectedCameraPosition[3] = {0.0f, 0.0f, 0.0f};
    const std::vector<ShotMetaView>* selectedShotMeta = nullptr;
    const std::vector<std::string>* importDiagnostics = nullptr;
    const ShotHudView* shotHud = nullptr;
    const char* exampleSkillPath = "";
    bool canUndo = false;
    bool canRedo = false;
    bool gizmoActive = false;
    float gizmoView[16] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
    float gizmoProj[16] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
    float gizmoWorld[16] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
    bool canGenerateAi = false;
    bool canRunSelectedSkill = false;
    const char* aiProvider = "openai-compat";
    const char* aiBaseUrl = "";
    const char* aiImageModel = "";
    const char* aiVideoModel = "";
    const char* aiChatModel = "";
    bool aiHasApiKey = false;
    bool aiBusy = false;
    const char* aiJobStatus = "";
    const char* aiJobMessage = "";
    float aiJobRatio = 0.0f;
    const char* lastAiOutputPath = "";
    bool projectIsEmpty = false;
    int scriptSelectedLineStart = 0;
    int scriptSelectedLineEnd = 0;
};

inline const StoryboardCardView* FindShotCardById(const std::vector<StoryboardCardView>* cards,
                                                  const std::string& shotId) {
    if (cards == nullptr || shotId.empty()) {
        return nullptr;
    }
    for (const StoryboardCardView& card : *cards) {
        if (card.kindEnum == CardKind::Shot && card.shotId == shotId) {
            return &card;
        }
    }
    return nullptr;
}

class IPanel {
public:
    virtual ~IPanel() = default;
    virtual void Draw(const AppViewState& state, Core::CommandQueue& commands) = 0;
};

} // namespace DirectorDesk::UI
