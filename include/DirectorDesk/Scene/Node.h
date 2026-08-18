#pragma once

#include "DirectorDesk/Scene/Transform.h"

#include <cstdint>
#include <string>

namespace DirectorDesk::Scene {

struct Node {
    std::string id;
    std::string name;
    Transform transform;
    std::uint32_t gpuModelId = 0;
    bool visible = true;
};

} // namespace DirectorDesk::Scene
