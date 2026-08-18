// LibraryPanel: Public or internal interface for the DirectorDesk UI module.
// This file owns project behavior only; keep platform and dependency boundaries explicit.

#pragma once

#include "DirectorDesk/UI/IPanel.h"

#include <string>

namespace DirectorDesk::UI {

class LibraryPanel final : public IPanel {
public:
    void Draw(const AppViewState& state, Core::CommandQueue& commands) override;

private:
    std::string m_search;
};

} // namespace DirectorDesk::UI
