// WorkspacePanel: Public or internal interface for the DirectorDesk UI module.
// This file owns project behavior only; keep platform and dependency boundaries explicit.

#pragma once

#include "DirectorDesk/UI/IPanel.h"

#include <cstdint>
#include <string>

namespace DirectorDesk::UI {

class WorkspacePanel final : public IPanel {
public:
    void Draw(const AppViewState& state, Core::CommandQueue& commands) override;

private:
    std::string m_cameraName;
    std::string m_cameraNameId;
    std::uint32_t m_lastViewportW = 0;
    std::uint32_t m_lastViewportH = 0;
};

} // namespace DirectorDesk::UI
