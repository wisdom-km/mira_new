// WorkspacePanel: Public or internal interface for the DirectorDesk UI module.
// This file owns project behavior only; keep platform and dependency boundaries explicit.

#pragma once

#include "DirectorDesk/UI/IPanel.h"

#include <cstdint>
#include <string>

namespace DirectorDesk::UI {

struct UiPreferences {
    bool lockExportAspect = true;
    bool leftFoldExplicit = false;
    bool leftFolded = false;
    bool showGroundGrid = true;
    bool showGroundAxes = true;
    bool showThirds = false;
    bool showSafeFrame = false;
    std::string viewportBackground = "neutral";
    float uiScale = 1.0f;
    bool openLastProject = false;
    std::string defaultExportDirectory;
    std::string defaultSkillId;
};

class WorkspacePanel final : public IPanel {
public:
    void Draw(const AppViewState& state, Core::CommandQueue& commands) override;
    void ApplyPreferences(const UiPreferences& preferences);
    [[nodiscard]] UiPreferences Preferences() const;
    bool ConsumePreferencesDirty();

private:
    void MarkPreferencesDirty();
    void DrawSettingsModal(const AppViewState& state, Core::CommandQueue& commands);
    void SyncSettingsAi(const AppViewState& state);
    void FlushSettingsAi(const AppViewState& state, Core::CommandQueue& commands);
    void DrawSkillsSettings(const AppViewState& state, Core::CommandQueue& commands);
    std::string m_cameraName;
    std::string m_cameraNameId;
    std::uint32_t m_lastViewportW = 0;
    std::uint32_t m_lastViewportH = 0;
    bool m_leftFoldExplicit = false;
    bool m_leftFolded = false;
    bool m_leftFoldDirty = false;
    bool m_lastIconBar = false;
    bool m_lastEmpty = true;
    bool m_lockExportAspect = true;
    bool m_showGroundGrid = true;
    bool m_showGroundAxes = true;
    bool m_showThirds = false;
    bool m_showSafeFrame = false;
    std::string m_viewportBackground = "neutral";
    bool m_preferencesDirty = false;
    bool m_sceneFacePinned = false;
    std::string m_lastSelectionKey;
    int m_gizmoOp = 0;
    float m_uiScale = 1.0f;
    bool m_settingsOpen = false;
    int m_settingsPage = 0;
    bool m_settingsAiSynced = false;
    bool m_showApiKey = false;
    bool m_openLastProject = false;
    char m_exportDirBuf[512] = {};
    char m_aiUrlBuf[256] = {};
    char m_aiKeyBuf[256] = {};
    char m_aiImageBuf[64] = {};
    char m_aiVideoBuf[64] = {};
    char m_aiChatBuf[64] = {};
    std::string m_aiProvider = "openai-compat";
    std::string m_defaultSkillId;
    std::string m_settingsSkillId;
    std::string m_skillPendingRemove;
    std::string m_scrollShotId;
    bool m_hotkeysOpen = false;
    std::string m_leftTabFocus;
};

} // namespace DirectorDesk::UI
