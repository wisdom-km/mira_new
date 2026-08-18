#pragma once

#include "DirectorDesk/Asset/ModelData.h"
#include "DirectorDesk/Core/Result.h"

#include <string>

namespace DirectorDesk::Asset {

class IModelLoader {
public:
    virtual ~IModelLoader() = default;
    [[nodiscard]] virtual bool CanLoad(const std::string& extensionUtf8) const = 0;
    virtual Core::Result<ModelData> Load(const std::string& utf8Path) const = 0;
};

} // namespace DirectorDesk::Asset
