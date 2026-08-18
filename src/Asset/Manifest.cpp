// Manifest: Implementation for the DirectorDesk Asset module.
// This file owns project behavior only; keep platform and dependency boundaries explicit.

#include "DirectorDesk/Asset/Manifest.h"

#include "DirectorDesk/Core/Error.h"

#include <nlohmann/json.hpp>

#include <cctype>
#include <unordered_set>

namespace DirectorDesk::Asset {
namespace {

bool IsLowerHex(const std::string& value, std::size_t length) {
    if (value.size() != length) {
        return false;
    }
    for (char ch : value) {
        if (!((ch >= '0' && ch <= '9') || (ch >= 'a' && ch <= 'f'))) {
            return false;
        }
    }
    return true;
}

bool IsSemVer(const std::string& value) {
    int part = 0;
    std::size_t i = 0;
    while (i < value.size() && part < 3) {
        if (value[i] < '0' || value[i] > '9') {
            return false;
        }
        while (i < value.size() && value[i] >= '0' && value[i] <= '9') {
            ++i;
        }
        ++part;
        if (part < 3) {
            if (i >= value.size() || value[i] != '.') {
                return false;
            }
            ++i;
        }
    }
    if (part != 3) {
        return false;
    }
    if (i == value.size()) {
        return true;
    }
    return value[i] == '-' || value[i] == '+';
}

bool IsAssetId(const std::string& value) {
    if (value.empty() || value.size() > 64) {
        return false;
    }
    if (!((value[0] >= 'a' && value[0] <= 'z') || (value[0] >= '0' && value[0] <= '9'))) {
        return false;
    }
    for (char ch : value) {
        if (!((ch >= 'a' && ch <= 'z') || (ch >= '0' && ch <= '9') || ch == '_' || ch == '-')) {
            return false;
        }
    }
    return true;
}

LocalizedText ReadLocalized(const nlohmann::json& value) {
    LocalizedText text;
    if (value.is_string()) {
        text.zhCN = value.get<std::string>();
        return text;
    }
    if (!value.is_object()) {
        return text;
    }
    text.zhCN = value.value("zh-CN", "");
    text.en = value.value("en", "");
    return text;
}

bool HasName(const LocalizedText& text) {
    return !text.zhCN.empty() || !text.en.empty();
}

const ManifestFile* FindFile(const std::vector<ManifestFile>& files, const std::string& path) {
    for (const ManifestFile& file : files) {
        if (file.path == path) {
            return &file;
        }
    }
    return nullptr;
}

bool PreviewOk(const std::string& path) {
    const std::size_t dot = path.find_last_of('.');
    if (dot == std::string::npos) {
        return false;
    }
    std::string ext = path.substr(dot);
    for (char& ch : ext) {
        ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
    }
    return ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".webp";
}

} // namespace

std::string PickLocale(const LocalizedText& text) {
    if (!text.zhCN.empty()) {
        return text.zhCN;
    }
    return text.en;
}

bool IsSafeRelativePath(const std::string& path) {
    if (path.empty() || path[0] == '/' || path[0] == '\\') {
        return false;
    }
    if (path.find("://") != std::string::npos || path.find(':') != std::string::npos) {
        return false;
    }
    if (path.find('\\') != std::string::npos || path.find("..") != std::string::npos) {
        return false;
    }
    return true;
}

Core::Result<std::string> JoinOfficialUrl(const std::string& httpsBase,
                                          const std::string& relative) {
    if (httpsBase.rfind("https://", 0) != 0) {
        return Core::Result<std::string>::Fail(Core::Error::Make(
            Core::ErrorCode::InvalidArgument, "Official base must be https", "官方地址必须是 HTTPS"));
    }
    if (!IsSafeRelativePath(relative)) {
        return Core::Result<std::string>::Fail(Core::Error::Make(
            Core::ErrorCode::InvalidArgument, "Unsafe official relative URL", "官方资源路径非法"));
    }
    std::string base = httpsBase;
    while (!base.empty() && base.back() == '/') {
        base.pop_back();
    }
    std::string url = base + "/" + relative;
    if (url.rfind(base, 0) != 0) {
        return Core::Result<std::string>::Fail(Core::Error::Make(
            Core::ErrorCode::InvalidArgument, "Resolved URL left official host", "官方资源越界"));
    }
    return Core::Result<std::string>::Ok(std::move(url));
}

ManifestParseResult ParseManifest(const std::string& jsonText) {
    ManifestParseResult result;
    try {
        const nlohmann::json root = nlohmann::json::parse(jsonText);
        if (!root.is_object()) {
            result.diagnostics.emplace_back("root is not an object");
            return result;
        }
        if (!root.contains("schemaVersion") || !root["schemaVersion"].is_number_integer() ||
            root["schemaVersion"].get<int>() != 1) {
            result.diagnostics.emplace_back("unsupported schemaVersion");
            return result;
        }
        if (!root.contains("revision") || !root["revision"].is_string() ||
            !root.contains("categories") || !root["categories"].is_array() ||
            !root.contains("assets") || !root["assets"].is_array()) {
            result.diagnostics.emplace_back("missing required root fields");
            return result;
        }
        result.revision = root["revision"].get<std::string>();
        result.generatedAt = root.value("generatedAt", "");

        std::unordered_set<std::string> categoryIds;
        for (const nlohmann::json& item : root["categories"]) {
            if (!item.is_object() || !item.contains("id") || !item["id"].is_string()) {
                result.diagnostics.emplace_back("invalid category");
                continue;
            }
            ManifestCategory category;
            category.id = item["id"].get<std::string>();
            if (item.contains("name")) {
                category.name = ReadLocalized(item["name"]);
            }
            if (category.id.empty() || categoryIds.count(category.id) != 0) {
                result.diagnostics.emplace_back("duplicate or empty category");
                continue;
            }
            categoryIds.insert(category.id);
            result.categories.push_back(std::move(category));
        }

        std::unordered_set<std::string> assetKeys;
        for (const nlohmann::json& item : root["assets"]) {
            if (!item.is_object()) {
                result.diagnostics.emplace_back("asset is not an object");
                continue;
            }
            ManifestAsset asset;
            asset.id = item.value("id", "");
            asset.version = item.value("version", "");
            if (item.contains("name")) {
                asset.name = ReadLocalized(item["name"]);
            }
            if (item.contains("description")) {
                asset.description = ReadLocalized(item["description"]);
            }
            asset.category = item.value("category", "");
            asset.format = item.value("format", "");
            asset.entrypoint = item.value("entrypoint", "");
            asset.preview = item.value("preview", "");
            if (item.contains("license") && item["license"].is_object()) {
                asset.license.spdx = item["license"].value("spdx", "");
                asset.license.name = item["license"].value("name", "");
                asset.license.url = item["license"].value("url", "");
                asset.license.attribution = item["license"].value("attribution", "");
            }
            if (item.contains("author") && item["author"].is_object()) {
                asset.author.name = item["author"].value("name", "");
                asset.author.url = item["author"].value("url", "");
            }
            if (item.contains("tags") && item["tags"].is_array()) {
                std::unordered_set<std::string> tags;
                for (const nlohmann::json& tag : item["tags"]) {
                    if (!tag.is_string()) {
                        continue;
                    }
                    std::string value = tag.get<std::string>();
                    for (char& ch : value) {
                        ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
                    }
                    if (tags.insert(value).second) {
                        asset.tags.push_back(value);
                    }
                }
            }
            if (item.contains("files") && item["files"].is_array()) {
                for (const nlohmann::json& fileJson : item["files"]) {
                    if (!fileJson.is_object()) {
                        continue;
                    }
                    ManifestFile file;
                    file.path = fileJson.value("path", "");
                    file.url = fileJson.value("url", "");
                    file.sha256 = fileJson.value("sha256", "");
                    file.size = fileJson.value("size", 0ull);
                    asset.files.push_back(std::move(file));
                }
            }

            if (!IsAssetId(asset.id) || !IsSemVer(asset.version) || !HasName(asset.name) ||
                categoryIds.count(asset.category) == 0 ||
                (asset.format != "glb" && asset.format != "obj") || asset.files.empty() ||
                FindFile(asset.files, asset.entrypoint) == nullptr ||
                (asset.license.spdx != "CC0-1.0" && asset.license.spdx != "CC-BY-4.0") ||
                asset.author.name.empty()) {
                result.diagnostics.emplace_back("rejected asset " + asset.id);
                continue;
            }
            if (asset.license.spdx == "CC-BY-4.0" && asset.license.attribution.empty()) {
                result.diagnostics.emplace_back("rejected asset " + asset.id + " missing attribution");
                continue;
            }
            const std::string key = asset.id + "@" + asset.version;
            if (assetKeys.count(key) != 0) {
                result.diagnostics.emplace_back("duplicate asset " + key);
                continue;
            }
            bool filesOk = true;
            for (const ManifestFile& file : asset.files) {
                if (!IsSafeRelativePath(file.path) || !IsSafeRelativePath(file.url) ||
                    !IsLowerHex(file.sha256, 64)) {
                    filesOk = false;
                    break;
                }
            }
            if (!filesOk || (!asset.preview.empty() && FindFile(asset.files, asset.preview) == nullptr) ||
                (!asset.preview.empty() && !PreviewOk(asset.preview))) {
                result.diagnostics.emplace_back("rejected asset files " + asset.id);
                continue;
            }
            const ManifestFile* entry = FindFile(asset.files, asset.entrypoint);
            const std::string expectedExt = asset.format == "glb" ? ".glb" : ".obj";
            if (entry == nullptr ||
                entry->path.size() < expectedExt.size() ||
                entry->path.compare(entry->path.size() - expectedExt.size(), expectedExt.size(),
                                    expectedExt) != 0) {
                result.diagnostics.emplace_back("rejected asset entrypoint " + asset.id);
                continue;
            }
            assetKeys.insert(key);
            result.assets.push_back(std::move(asset));
        }
        result.accepted = true;
    } catch (const nlohmann::json::exception& ex) {
        result.diagnostics.emplace_back(ex.what());
        result.accepted = false;
    }
    return result;
}

} // namespace DirectorDesk::Asset
