// Loaders: Public or internal interface for the DirectorDesk Asset module.
// This file owns project behavior only; keep platform and dependency boundaries explicit.

#pragma once

#include "DirectorDesk/Asset/IModelLoader.h"

#include <memory>

namespace DirectorDesk::Asset {

std::unique_ptr<IModelLoader> CreateGlbLoader();
std::unique_ptr<IModelLoader> CreateObjLoader();

} // namespace DirectorDesk::Asset
