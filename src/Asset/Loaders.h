#pragma once

#include "DirectorDesk/Asset/IModelLoader.h"

#include <memory>

namespace DirectorDesk::Asset {

std::unique_ptr<IModelLoader> CreateGlbLoader();
std::unique_ptr<IModelLoader> CreateObjLoader();

} // namespace DirectorDesk::Asset
