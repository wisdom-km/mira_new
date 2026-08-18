#pragma once

#include "DirectorDesk/UI/IPanel.h"

namespace DirectorDesk::UI {

class WorkspacePanel final : public IPanel {
public:
    void Draw(const AppViewState& state, Core::CommandQueue& commands) override;
};

} // namespace DirectorDesk::UI
