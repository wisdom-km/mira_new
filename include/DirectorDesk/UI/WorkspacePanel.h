#pragma once

#include "DirectorDesk/UI/IPanel.h"

#include <string>

namespace DirectorDesk::UI {

class WorkspacePanel final : public IPanel {
public:
    void Draw(const AppViewState& state, Core::CommandQueue& commands) override;

private:
    std::string m_cameraName;
    std::string m_cameraNameId;
};

} // namespace DirectorDesk::UI
