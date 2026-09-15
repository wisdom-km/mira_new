// ViewStateBuilder: Per-frame AppViewState assembly (FND-10 / CR-10).
#pragma once

#include "DirectorDesk/App/AppState.h"
#include "DirectorDesk/UI/IPanel.h"

#include <string>
#include <unordered_map>
#include <vector>

namespace DirectorDesk::App {

struct FrameStrings {
    std::vector<UI::NodeView> nodes;
    std::vector<UI::ScriptSceneView> scriptScenes;
    std::vector<UI::ScriptDiagnosticView> scriptDiagnostics;
    std::vector<UI::CameraItemView> cameras;
    std::vector<UI::LibraryAssetView> libraryAssets;
    std::vector<std::string> officialCategories;
    std::vector<UI::StoryboardCardView> storyboardCards;
    std::vector<UI::ExportIssueView> exportIssues;
    std::vector<UI::ShotMetaView> selectedShotMeta;
    std::vector<std::string> importDiagnostics;
    std::string selectedShotLinkedCamera;
    std::unordered_map<std::string, std::string> libraryPreviewPaths;
    UI::ShotHudView shotHud;
    std::string shotHudTitle;
    std::string shotHudCamera;
};

void BuildViewState(AppState& state, UI::AppViewState& viewState, FrameStrings& frame);

} // namespace DirectorDesk::App
