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
};

class IPanel {
public:
    virtual ~IPanel() = default;
    virtual void Draw(const AppViewState& state, Core::CommandQueue& commands) = 0;
};

} // namespace DirectorDesk::UI
