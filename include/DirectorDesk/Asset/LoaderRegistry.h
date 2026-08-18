// LoaderRegistry: Public or internal interface for the DirectorDesk Asset module.
// This file owns project behavior only; keep platform and dependency boundaries explicit.

#pragma once

#include "DirectorDesk/Asset/IModelLoader.h"

#include <memory>
#include <vector>

namespace DirectorDesk::Asset {

class LoaderRegistry {
public:
    void Add(std::unique_ptr<IModelLoader> loader);
    [[nodiscard]] const IModelLoader* FindByExtension(const std::string& extensionUtf8) const;
    Core::Result<ModelData> Load(const std::string& utf8Path) const;

private:
    std::vector<std::unique_ptr<IModelLoader>> m_loaders;
};

LoaderRegistry CreateDefaultRegistry();

} // namespace DirectorDesk::Asset
