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
};

class WorkspacePanel final : public IPanel {
public:
    void Draw(const AppViewState& state, Core::CommandQueue& commands) override;
    void ApplyPreferences(const UiPreferences& preferences);
    [[nodiscard]] UiPreferences Preferences() const;
    bool ConsumePreferencesDirty();

private:
    void MarkPreferencesDirty();
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
};

} // namespace DirectorDesk::UI
