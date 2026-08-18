// ModelLoadResult: Public or internal interface for the DirectorDesk Asset module.
// This file owns project behavior only; keep platform and dependency boundaries explicit.

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
