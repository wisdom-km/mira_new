// LoaderRegistry: Implementation for the DirectorDesk Asset module.
// This file owns project behavior only; keep platform and dependency boundaries explicit.

#include "DirectorDesk/Asset/LoaderRegistry.h"

#include "Loaders.h"

#include "DirectorDesk/Platform/Paths.h"

namespace DirectorDesk::Asset {

void LoaderRegistry::Add(std::unique_ptr<IModelLoader> loader) {
    if (loader != nullptr) {
        m_loaders.push_back(std::move(loader));
    }
}

const IModelLoader* LoaderRegistry::FindByExtension(const std::string& extensionUtf8) const {
    for (const auto& loader : m_loaders) {
        if (loader->CanLoad(extensionUtf8)) {
            return loader.get();
        }
    }
    return nullptr;
}

Core::Result<ModelData> LoaderRegistry::Load(const std::string& utf8Path) const {
    const std::string extension = Platform::Paths::ExtensionLower(utf8Path);
    const IModelLoader* loader = FindByExtension(extension);
    if (loader == nullptr) {
        return Core::Result<ModelData>::Fail(Core::Error::Make(
            Core::ErrorCode::Unsupported, "No loader registered for extension " + extension,
            "暂不支持该模型格式"));
    }
    return loader->Load(utf8Path);
}

LoaderRegistry CreateDefaultRegistry() {
    LoaderRegistry registry;
    registry.Add(CreateGlbLoader());
    registry.Add(CreateObjLoader());
    return registry;
}

} // namespace DirectorDesk::Asset
