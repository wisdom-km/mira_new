// StoryboardPanel: Public or internal interface for the DirectorDesk UI module.
// This file owns project behavior only; keep platform and dependency boundaries explicit.

#pragma once

#include "DirectorDesk/UI/IPanel.h"

namespace DirectorDesk::UI {

class StoryboardPanel final : public IPanel {
public:
    void Draw(const AppViewState& state, Core::CommandQueue& commands) override;

private:
    float m_panX = 32.0f;
    float m_panY = 32.0f;
    float m_zoom = 1.0f;
};

} // namespace DirectorDesk::UI
