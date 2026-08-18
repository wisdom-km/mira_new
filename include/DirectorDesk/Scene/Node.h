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
    std::string assetRef;
    std::string parent;
    std::string sourcePath;
    std::string libraryAssetId;
    bool assetMissing = false;
};

} // namespace DirectorDesk::Scene
