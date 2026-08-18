#pragma once

#include "DirectorDesk/Asset/ModelData.h"
#include "DirectorDesk/Core/Error.h"

#include <string>

namespace DirectorDesk::Asset {

struct ModelLoadResult {
    std::string sourcePath;
    bool ok = false;
    ModelData model;
    Core::Error error;
};

} // namespace DirectorDesk::Asset
