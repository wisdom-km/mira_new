// Library: Public or internal interface for the DirectorDesk Asset module.
// This file owns project behavior only; keep platform and dependency boundaries explicit.

#pragma once

#include "DirectorDesk/Core/Result.h"

#include <cstdint>
#include <string>
#include <vector>

namespace DirectorDesk::Asset {

enum class AssetOrigin {
    Builtin,
    User,
    OnlineCache,
};

struct LibraryAsset {
    std::string id;
    std::string name;
    std::string sourcePath;
    std::string format;
    AssetOrigin origin = AssetOrigin::User;
    std::string category = "uncategorized";
    std::vector<std::string> tags;
    std::string previewPath;
    std::uint64_t fileSize = 0;
    bool sourceExists = true;
};

class Library {
public:
    Core::Result<void> Open(const std::string& directoryUtf8);
    Core::Result<void> Save() const;
    void Refresh();

    Core::Result<LibraryAsset> Import(const std::string& utf8Path, AssetOrigin origin);
    Core::Result<LibraryAsset> Upsert(LibraryAsset asset);
    bool SetPreviewPath(const std::string& assetId, std::string previewPath);
    [[nodiscard]] const LibraryAsset* Find(const std::string& assetId) const;
    [[nodiscard]] std::vector<LibraryAsset> Query(const std::string& search,
                                                  const std::string& originFilter) const;
    [[nodiscard]] const std::vector<LibraryAsset>& Assets() const {
        return m_assets;
    }
    [[nodiscard]] const std::string& Directory() const {
        return m_directory;
    }
    [[nodiscard]] bool RecoveredFromCorruptIndex() const {
        return m_recoveredFromCorrupt;
    }

    static std::string MakeId(const std::string& utf8Path);
    static const char* OriginId(AssetOrigin origin);
    static bool TryParseOrigin(const std::string& id, AssetOrigin& out);

private:
    Core::Result<void> LoadIndex();
    [[nodiscard]] const LibraryAsset* FindByKey(const std::string& key) const;
    LibraryAsset* FindMutable(const std::string& assetId);

    std::string m_directory;
    std::string m_indexPath;
    std::vector<LibraryAsset> m_assets;
    bool m_recoveredFromCorrupt = false;
};

} // namespace DirectorDesk::Asset
