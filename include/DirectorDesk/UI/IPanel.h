#pragma once

#include "DirectorDesk/Core/CommandQueue.h"

namespace DirectorDesk::UI {

struct AppViewState {
    const char* appName = "DirectorDesk";
    unsigned windowWidth = 0;
    unsigned windowHeight = 0;
};

class IPanel {
public:
    virtual ~IPanel() = default;
    virtual void Draw(const AppViewState& state, Core::CommandQueue& commands) = 0;
};

} // namespace DirectorDesk::UI
