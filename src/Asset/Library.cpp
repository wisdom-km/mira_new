#include "DirectorDesk/Asset/Library.h"

#include "DirectorDesk/Core/Error.h"
#include "DirectorDesk/Platform/Paths.h"

#include <nlohmann/json.hpp>

#include <cstdint>
#include <sstream>

namespace DirectorDesk::Asset {
namespace {

std::string ToLower(std::string value) {
    for (char& ch : value) {
        if (ch >= 'A' && ch <= 'Z') {
            ch = static_cast<char>(ch - 'A' + 'a');
        }
    }
    return value;
}

bool ContainsInsensitive(const std::string& haystack, const std::string& needle) {
    if (needle.empty()) {
        return true;
    }
    const std::string left = ToLower(haystack);
    const std::string right = ToLower(needle);
    return left.find(right) != std::string::npos;
}

std::string Fnv1aHex(const std::string& text) {
    std::uint64_t hash = 14695981039346656037ull;
    for (unsigned char byte : text) {
        hash ^= static_cast<std::uint64_t>(byte);
        hash *= 1099511628211ull;
    }
    std::ostringstream out;
    out << std::hex;
    for (int shift = 60; shift >= 0; shift -= 4) {
        out << ((hash >> shift) & 0xfull);
    }
    return out.str();
}

const char* OriginJson(AssetOrigin origin) {
    return Library::OriginId(origin);
}

} // namespace

const char* Library::OriginId(AssetOrigin origin) {
    switch (origin) {
    case AssetOrigin::Builtin:
        return "builtin";
    case AssetOrigin::User:
        return "user";
    case AssetOrigin::OnlineCache:
        return "online";
    }
    return "user";
}

bool Library::TryParseOrigin(const std::string& id, AssetOrigin& out) {
    if (id == "builtin") {
        out = AssetOrigin::Builtin;
        return true;
    }
    if (id == "user") {
        out = AssetOrigin::User;
        return true;
    }
    if (id == "online") {
        out = AssetOrigin::OnlineCache;
        return true;
    }
    return false;
}

std::string Library::MakeId(const std::string& utf8Path) {
    return "local-" + Fnv1aHex(Platform::Paths::StableKey(utf8Path));
}

Core::Result<void> Library::Open(const std::string& directoryUtf8) {
    auto created = Platform::Paths::CreateDirectories(directoryUtf8);
    if (!created.IsOk()) {
        return created;
    }
    const std::string previews = Platform::Paths::Join(directoryUtf8, "previews");
    auto previewDir = Platform::Paths::CreateDirectories(previews);
    if (!previewDir.IsOk()) {
        return previewDir;
    }
    m_directory = directoryUtf8;
    m_indexPath = Platform::Paths::Join(directoryUtf8, "index.json");
    return LoadIndex();
}

Core::Result<void> Library::LoadIndex() {
    m_assets.clear();
    m_recoveredFromCorrupt = false;
    if (!Platform::Paths::Exists(m_indexPath)) {
        return Core::Result<void>::Ok();
    }

    auto text = Platform::Paths::ReadTextFile(m_indexPath);
    if (!text.IsOk()) {
        m_recoveredFromCorrupt = true;
        return Core::Result<void>::Ok();
    }

    try {
        const nlohmann::json root = nlohmann::json::parse(text.Value());
        if (!root.is_object() || !root.contains("schemaVersion") ||
            !root["schemaVersion"].is_number_integer() || root["schemaVersion"].get<int>() != 1) {
            m_recoveredFromCorrupt = true;
            return Core::Result<void>::Ok();
        }
        if (!root.contains("assets") || !root["assets"].is_array()) {
            return Core::Result<void>::Ok();
        }
        for (const nlohmann::json& item : root["assets"]) {
            if (!item.is_object() || !item.contains("id") || !item.contains("sourcePath") ||
                !item.contains("format")) {
                continue;
            }
            LibraryAsset asset;
            asset.id = item.value("id", "");
            asset.name = item.value("name", "");
            asset.sourcePath = item.value("sourcePath", "");
            asset.format = item.value("format", "");
            AssetOrigin origin = AssetOrigin::User;
            TryParseOrigin(item.value("origin", "user"), origin);
            asset.origin = origin;
            asset.category = item.value("category", "uncategorized");
            asset.previewPath = item.value("previewPath", "");
            asset.fileSize = item.value("fileSize", 0ull);
            if (item.contains("tags") && item["tags"].is_array()) {
                for (const nlohmann::json& tag : item["tags"]) {
                    if (tag.is_string()) {
                        asset.tags.push_back(tag.get<std::string>());
                    }
                }
            }
            if (asset.id.empty() || asset.sourcePath.empty()) {
                continue;
            }
            asset.sourceExists = Platform::Paths::Exists(asset.sourcePath);
            m_assets.push_back(std::move(asset));
        }
    } catch (const nlohmann::json::exception&) {
        m_recoveredFromCorrupt = true;
        m_assets.clear();
    }
    return Core::Result<void>::Ok();
}

Core::Result<void> Library::Save() const {
    if (m_indexPath.empty()) {
        return Core::Result<void>::Fail(Core::Error::Make(
            Core::ErrorCode::NotInitialized, "Library is not open", "资源库尚未打开"));
    }
    nlohmann::json root;
    root["schemaVersion"] = 1;
    nlohmann::json assets = nlohmann::json::array();
    for (const LibraryAsset& asset : m_assets) {
        nlohmann::json item;
        item["id"] = asset.id;
        item["name"] = asset.name;
        item["sourcePath"] = asset.sourcePath;
        item["format"] = asset.format;
        item["origin"] = OriginJson(asset.origin);
        item["category"] = asset.category;
        item["previewPath"] = asset.previewPath;
        item["fileSize"] = asset.fileSize;
        item["tags"] = asset.tags;
        assets.push_back(std::move(item));
    }
    root["assets"] = std::move(assets);
    return Platform::Paths::WriteTextFile(m_indexPath, root.dump(2));
}

void Library::Refresh() {
    for (LibraryAsset& asset : m_assets) {
        asset.sourceExists = Platform::Paths::Exists(asset.sourcePath);
        if (asset.sourceExists) {
            auto size = Platform::Paths::FileSize(asset.sourcePath);
            if (size.IsOk()) {
                asset.fileSize = size.Value();
            }
        }
    }
}

const LibraryAsset* Library::FindByKey(const std::string& key) const {
    for (const LibraryAsset& asset : m_assets) {
        if (Platform::Paths::StableKey(asset.sourcePath) == key) {
            return &asset;
        }
    }
    return nullptr;
}

const LibraryAsset* Library::Find(const std::string& assetId) const {
    for (const LibraryAsset& asset : m_assets) {
        if (asset.id == assetId) {
            return &asset;
        }
    }
    return nullptr;
}

LibraryAsset* Library::FindMutable(const std::string& assetId) {
    for (LibraryAsset& asset : m_assets) {
        if (asset.id == assetId) {
            return &asset;
        }
    }
    return nullptr;
}

Core::Result<LibraryAsset> Library::Import(const std::string& utf8Path, AssetOrigin origin) {
    if (m_directory.empty()) {
        return Core::Result<LibraryAsset>::Fail(Core::Error::Make(
            Core::ErrorCode::NotInitialized, "Library is not open", "资源库尚未打开"));
    }
    if (!Platform::Paths::Exists(utf8Path) || Platform::Paths::IsDirectory(utf8Path)) {
        return Core::Result<LibraryAsset>::Fail(
            Core::Error::Make(Core::ErrorCode::NotFound, "Asset file does not exist", "源文件不存在"));
    }
    const std::string extension = Platform::Paths::ExtensionLower(utf8Path);
    if (extension != ".glb" && extension != ".obj") {
        return Core::Result<LibraryAsset>::Fail(Core::Error::Make(
            Core::ErrorCode::Unsupported, "Unsupported library format", "资源库只支持 GLB/OBJ"));
    }

    const std::string key = Platform::Paths::StableKey(utf8Path);
    if (const LibraryAsset* existing = FindByKey(key)) {
        return Core::Result<LibraryAsset>::Ok(*existing);
    }

    LibraryAsset asset;
    asset.id = MakeId(utf8Path);
    asset.name = Platform::Paths::Stem(utf8Path);
    auto canonical = Platform::Paths::WeaklyCanonical(utf8Path);
    asset.sourcePath = canonical.IsOk() ? canonical.Value() : utf8Path;
    asset.format = extension == ".glb" ? "glb" : "obj";
    asset.origin = origin;
    asset.category = origin == AssetOrigin::Builtin ? "builtin" : "uncategorized";
    asset.sourceExists = true;
    auto size = Platform::Paths::FileSize(utf8Path);
    if (size.IsOk()) {
        asset.fileSize = size.Value();
    }

    const std::string sidecarPng =
        Platform::Paths::Join(Platform::Paths::Parent(utf8Path), asset.name + ".png");
    if (Platform::Paths::Exists(sidecarPng)) {
        const std::string cached =
            Platform::Paths::Join(Platform::Paths::Join(m_directory, "previews"), asset.id + ".png");
        auto bytes = Platform::Paths::ReadBinaryFile(sidecarPng);
        if (bytes.IsOk() &&
            Platform::Paths::WriteBinaryFile(cached, bytes.Value().data(), bytes.Value().size())
                .IsOk()) {
            asset.previewPath = cached;
        }
    }

    m_assets.push_back(asset);
    auto saved = Save();
    if (!saved.IsOk()) {
        m_assets.pop_back();
        return Core::Result<LibraryAsset>::Fail(saved.GetError());
    }
    return Core::Result<LibraryAsset>::Ok(std::move(asset));
}

Core::Result<LibraryAsset> Library::Upsert(LibraryAsset asset) {
    if (m_directory.empty()) {
        return Core::Result<LibraryAsset>::Fail(Core::Error::Make(
            Core::ErrorCode::NotInitialized, "Library is not open", "资源库尚未打开"));
    }
    if (asset.id.empty() || asset.sourcePath.empty()) {
        return Core::Result<LibraryAsset>::Fail(Core::Error::Make(
            Core::ErrorCode::InvalidArgument, "Online asset is incomplete", "在线资产不完整"));
    }
    asset.sourceExists = Platform::Paths::Exists(asset.sourcePath);
    if (LibraryAsset* existing = FindMutable(asset.id)) {
        *existing = asset;
    } else {
        m_assets.push_back(asset);
    }
    auto saved = Save();
    if (!saved.IsOk()) {
        return Core::Result<LibraryAsset>::Fail(saved.GetError());
    }
    return Core::Result<LibraryAsset>::Ok(std::move(asset));
}

bool Library::SetPreviewPath(const std::string& assetId, std::string previewPath) {
    LibraryAsset* asset = FindMutable(assetId);
    if (asset == nullptr) {
        return false;
    }
    asset->previewPath = std::move(previewPath);
    Save();
    return true;
}

std::vector<LibraryAsset> Library::Query(const std::string& search,
                                         const std::string& originFilter) const {
    std::vector<LibraryAsset> result;
    AssetOrigin origin = AssetOrigin::User;
    const bool filterOrigin = originFilter != "all" && TryParseOrigin(originFilter, origin);
    for (const LibraryAsset& asset : m_assets) {
        if (filterOrigin && asset.origin != origin) {
            continue;
        }
        const bool nameHit = ContainsInsensitive(asset.name, search);
        const bool pathHit = ContainsInsensitive(Platform::Paths::FileName(asset.sourcePath), search);
        bool tagHit = false;
        for (const std::string& tag : asset.tags) {
            tagHit = tagHit || ContainsInsensitive(tag, search);
        }
        if (nameHit || pathHit || tagHit) {
            result.push_back(asset);
        }
    }
    return result;
}

} // namespace DirectorDesk::Asset
