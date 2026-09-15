// Library: Implementation for the DirectorDesk Asset module.
// This file owns project behavior only; keep platform and dependency boundaries explicit.

#include "DirectorDesk/Asset/Library.h"

#include "DirectorDesk/Core/Error.h"
#include "DirectorDesk/Platform/Paths.h"

#include <nlohmann/json.hpp>

#include <cstdint>
#include <sstream>
#include <cstddef>

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
    switch (origin) {
    case AssetOrigin::OnlineCache:
        return "official";
    case AssetOrigin::Builtin:
        return "builtin";
    case AssetOrigin::User:
    default:
        return "local";
    }
}

bool FingerprintMatches(const std::string& sourcePath, std::uint64_t sourceMtime,
                        std::uint64_t sourceSize) {
    if (sourcePath.empty() || sourceMtime == 0) {
        return false;
    }
    auto mtime = Platform::Paths::LastWriteTimeCount(sourcePath);
    auto size = Platform::Paths::FileSize(sourcePath);
    return mtime.IsOk() && size.IsOk() && mtime.Value() == sourceMtime && size.Value() == sourceSize;
}

void StampFingerprint(LibraryAsset& asset, const std::string& sourcePath) {
    auto mtime = Platform::Paths::LastWriteTimeCount(sourcePath);
    if (mtime.IsOk()) {
        asset.sourceMtime = mtime.Value();
    }
    auto size = Platform::Paths::FileSize(sourcePath);
    if (size.IsOk()) {
        asset.fileSize = size.Value();
    }
}

void CopySkillSidecars(const std::string& srcSkillPath, const std::string& destDir) {
    const std::string srcDir = Platform::Paths::Parent(srcSkillPath);
    const char* names[] = {"skill.json", "storyboard-import.json", "prompt.md"};
    for (const char* name : names) {
        const std::string src = Platform::Paths::Join(srcDir, name);
        if (Platform::Paths::Exists(src)) {
            auto copied = Platform::Paths::CopyFileUtf8(src, Platform::Paths::Join(destDir, name));
            (void)copied;
        }
    }
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
    if (id == "online" || id == "official") {
        out = AssetOrigin::OnlineCache;
        return true;
    }
    if (id == "local") {
        out = AssetOrigin::User;
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
            TryParseOrigin(item.value("origin", "local"), origin);
            asset.origin = origin;
            asset.category = item.value("category", "uncategorized");
            asset.previewPath = item.value("previewPath", "");
            asset.fileSize = item.value("sourceSize", item.value("fileSize", 0ull));
            asset.sourceMtime = item.value("sourceMtime", 0ull);
            asset.sha256 = item.value("sha256", "");
            asset.version = item.value("version", "");
            asset.entrypoint = item.value("entrypoint", "");
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
        item["sourceSize"] = asset.fileSize;
        item["sourceMtime"] = asset.sourceMtime;
        if (!asset.sha256.empty()) {
            item["sha256"] = asset.sha256;
        }
        if (!asset.version.empty()) {
            item["version"] = asset.version;
        }
        if (!asset.entrypoint.empty()) {
            item["entrypoint"] = asset.entrypoint;
        }
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
            StampFingerprint(asset, asset.sourcePath);
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

const LibraryAsset* Library::FindBySourcePath(const std::string& sourcePath) const {
    if (sourcePath.empty()) {
        return nullptr;
    }
    return FindByKey(Platform::Paths::StableKey(sourcePath));
}

bool Library::TryCachedHash(const std::string& sourcePath, const std::string& assetId,
                             std::string& sha256) const {
    const LibraryAsset* asset = nullptr;
    if (!assetId.empty()) {
        asset = Find(assetId);
    }
    if (asset == nullptr && !sourcePath.empty()) {
        asset = FindByKey(Platform::Paths::StableKey(sourcePath));
    }
    if (asset != nullptr && !asset->sha256.empty() &&
        FingerprintMatches(sourcePath.empty() ? asset->sourcePath : sourcePath, asset->sourceMtime,
                            asset->fileSize)) {
        sha256 = asset->sha256;
        return true;
    }
    const std::string key = Platform::Paths::StableKey(sourcePath);
    const auto found = m_ephemeralHashes.find(key);
    if (found != m_ephemeralHashes.end() && !found->second.sha256.empty() &&
        FingerprintMatches(sourcePath, found->second.sourceMtime, found->second.fileSize)) {
        sha256 = found->second.sha256;
        return true;
    }
    return false;
}

bool Library::RecordContentHash(const std::string& sourcePath, const std::string& assetId,
                                 const std::string& sha256) {
    if (sourcePath.empty() || sha256.empty()) {
        return false;
    }
    LibraryAsset* asset = nullptr;
    if (!assetId.empty()) {
        asset = FindMutable(assetId);
    }
    if (asset == nullptr) {
        if (const LibraryAsset* byKey = FindByKey(Platform::Paths::StableKey(sourcePath))) {
            asset = FindMutable(byKey->id);
        }
    }
    LibraryAsset fingerprint;
    fingerprint.sourcePath = sourcePath;
    fingerprint.sha256 = sha256;
    StampFingerprint(fingerprint, sourcePath);
    m_ephemeralHashes[Platform::Paths::StableKey(sourcePath)] = fingerprint;
    if (asset != nullptr) {
        asset->sha256 = sha256;
        asset->sourceMtime = fingerprint.sourceMtime;
        asset->fileSize = fingerprint.fileSize;
        if (!m_indexPath.empty()) {
            Save();
        }
    }
    return true;
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
    const std::string fileName = Platform::Paths::FileName(utf8Path);
    const bool skill = fileName == "SKILL.md" || fileName == "skill.md";
    if (extension != ".glb" && extension != ".obj" && !skill) {
        return Core::Result<LibraryAsset>::Fail(Core::Error::Make(
            Core::ErrorCode::Unsupported, "Unsupported library format",
            "资源库只支持 GLB/OBJ/SKILL.md"));
    }

    const std::string key = Platform::Paths::StableKey(utf8Path);
    if (const LibraryAsset* existing = FindByKey(key)) {
        if (skill) {
            CopySkillSidecars(utf8Path, Platform::Paths::Parent(existing->sourcePath));
        }
        return Core::Result<LibraryAsset>::Ok(*existing);
    }

    LibraryAsset asset;
    asset.id = MakeId(utf8Path);
    if (const LibraryAsset* existing = Find(asset.id)) {
        if (skill) {
            CopySkillSidecars(utf8Path, Platform::Paths::Parent(existing->sourcePath));
        }
        return Core::Result<LibraryAsset>::Ok(*existing);
    }
    asset.name = Platform::Paths::Stem(utf8Path);
    auto canonical = Platform::Paths::WeaklyCanonical(utf8Path);
    asset.sourcePath = canonical.IsOk() ? canonical.Value() : utf8Path;
    asset.format = skill ? "skill" : (extension == ".glb" ? "glb" : "obj");
    asset.origin = origin;
    asset.category = skill ? "skill" : (origin == AssetOrigin::Builtin ? "builtin" : "uncategorized");
    asset.entrypoint = skill ? "SKILL.md" : "";
    if (skill) {
        const std::string folder = Platform::Paths::FileName(Platform::Paths::Parent(utf8Path));
        asset.name = folder.empty() ? "Skill" : folder;
        if (folder == "storyboard") {
            asset.name = "分镜 Skill";
        }
        const std::string destDir =
            Platform::Paths::Join(Platform::Paths::Join(m_directory, "skills"), asset.id);
        const std::string destPath = Platform::Paths::Join(destDir, "SKILL.md");
        auto copied = Platform::Paths::CopyFileUtf8(utf8Path, destPath);
        if (!copied.IsOk()) {
            return Core::Result<LibraryAsset>::Fail(copied.GetError());
        }
        CopySkillSidecars(utf8Path, destDir);
        asset.sourcePath = destPath;
        asset.entrypoint = "SKILL.md";
    }
    asset.sourceExists = true;
    StampFingerprint(asset, asset.sourcePath);

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

bool Library::Remove(const std::string& assetId) {
    if (assetId.empty()) {
        return false;
    }
    std::size_t index = m_assets.size();
    for (std::size_t i = 0; i < m_assets.size(); ++i) {
        if (m_assets[i].id == assetId) {
            index = i;
            break;
        }
    }
    if (index >= m_assets.size()) {
        return false;
    }
    LibraryAsset removed = m_assets[index];
    m_assets.erase(m_assets.begin() + static_cast<std::ptrdiff_t>(index));
    auto saved = Save();
    if (!saved.IsOk()) {
        m_assets.insert(m_assets.begin() + static_cast<std::ptrdiff_t>(index), std::move(removed));
        return false;
    }
    return true;
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
