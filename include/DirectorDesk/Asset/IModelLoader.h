// IModelLoader: Public or internal interface for the DirectorDesk Asset module.
// This file owns project behavior only; keep platform and dependency boundaries explicit.

#pragma once

#include "DirectorDesk/Asset/ModelData.h"
#include "DirectorDesk/Core/Result.h"

#include <string>

namespace DirectorDesk::Asset {

class IModelLoader {
public:
    virtual ~IModelLoader() = default;
    // Loaders decode source files only; GPU upload remains an Application/Renderer concern.
    [[nodiscard]] virtual bool CanLoad(const std::string& extensionUtf8) const = 0;
    // Returns CPU-side geometry/material data or a localized failure result.
    virtual Core::Result<ModelData> Load(const std::string& utf8Path) const = 0;
};

} // namespace DirectorDesk::Asset
