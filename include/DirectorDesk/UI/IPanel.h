#pragma once

#include "DirectorDesk/Core/CommandQueue.h"

#include <cstdint>
#include <string>
#include <vector>

namespace DirectorDesk::UI {

struct NodeView {
    std::string id;
    std::string name;
    float position[3] = {0.0f, 0.0f, 0.0f};
    float eulerDegrees[3] = {0.0f, 0.0f, 0.0f};
    float scale[3] = {1.0f, 1.0f, 1.0f};
    bool selected = false;
};

struct ScriptShotView {
    std::string id;
    std::string title;
    std::string linkedCameraId;
    std::string linkedCameraName;
    bool linkedMissing = false;
    bool selected = false;
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
    bool missing = false;
    bool selected = false;
};

struct AppViewState {
    const char* appName = "DirectorDesk";
    unsigned windowWidth = 0;
    unsigned windowHeight = 0;
    std::uint16_t viewportTextureIndex = 0xFFFFu;
    unsigned viewportTextureWidth = 0;
    unsigned viewportTextureHeight = 0;
    const char* statusText = "";
    bool importInProgress = false;
    const std::vector<NodeView>* nodes = nullptr;
    const char* exampleObjPath = "";
    const char* exampleGlbPath = "";
    const char* exampleScriptPath = "";
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
    const char* libraryViewMode = "list";
    const char* projectName = "";
    const char* projectPath = "";
    bool projectDirty = false;
    bool projectPromptVisible = false;
    const char* selectedShotLinkedCamera = "";
};

class IPanel {
public:
    virtual ~IPanel() = default;
    virtual void Draw(const AppViewState& state, Core::CommandQueue& commands) = 0;
};

} // namespace DirectorDesk::UI
