// ProjectFile: Implementation for the DirectorDesk App module.
// This file owns project behavior only; keep platform and dependency boundaries explicit.

#include "DirectorDesk/App/ProjectFile.h"

#include "DirectorDesk/Core/Error.h"
#include "DirectorDesk/Platform/Paths.h"

#include <nlohmann/json.hpp>

#include <chrono>
#include <cmath>
#include <cstdint>
#include <sstream>
#include <unordered_set>

namespace DirectorDesk::App {
namespace {

Core::Error ParseError(const std::string& technical, const std::string& user) {
    return Core::Error::Make(Core::ErrorCode::ParseFailure, technical, user);
}

bool IsHexSha(const std::string& text) {
    if (text.size() != 64) {
        return false;
    }
    for (char ch : text) {
        const bool ok = (ch >= '0' && ch <= '9') || (ch >= 'a' && ch <= 'f');
        if (!ok) {
            return false;
        }
    }
    return true;
}

bool Finite3(const float* values) {
    return std::isfinite(values[0]) && std::isfinite(values[1]) && std::isfinite(values[2]);
}

bool ReadVec3(const nlohmann::json& item, const char* key, float out[3], bool required) {
    if (!item.contains(key)) {
        return !required;
    }
    const nlohmann::json& value = item[key];
    if (!value.is_array() || value.size() != 3 || !value[0].is_number() || !value[1].is_number() ||
        !value[2].is_number()) {
        return false;
    }
    out[0] = value[0].get<float>();
    out[1] = value[1].get<float>();
    out[2] = value[2].get<float>();
    return Finite3(out);
}

bool ReadQuat(const nlohmann::json& item, const char* key, float out[4]) {
    if (!item.contains(key)) {
        out[0] = 0.0f;
        out[1] = 0.0f;
        out[2] = 0.0f;
        out[3] = 1.0f;
        return true;
    }
    const nlohmann::json& value = item[key];
    if (!value.is_array() || value.size() != 4 || !value[0].is_number() || !value[1].is_number() ||
        !value[2].is_number() || !value[3].is_number()) {
        return false;
    }
    out[0] = value[0].get<float>();
    out[1] = value[1].get<float>();
    out[2] = value[2].get<float>();
    out[3] = value[3].get<float>();
    const float length = std::sqrt(out[0] * out[0] + out[1] * out[1] + out[2] * out[2] + out[3] * out[3]);
    if (!std::isfinite(length) || length < 1.0e-6f) {
        return false;
    }
    out[0] /= length;
    out[1] /= length;
    out[2] /= length;
    out[3] /= length;
    return true;
}

nlohmann::json Vec3Json(const float values[3]) {
    return nlohmann::json::array({values[0], values[1], values[2]});
}

nlohmann::json QuatJson(const float values[4]) {
    return nlohmann::json::array({values[0], values[1], values[2], values[3]});
}

bool HasCycle(const std::vector<ProjectNode>& nodes) {
    for (const ProjectNode& node : nodes) {
        std::unordered_set<std::string> seen;
        std::string current = node.parent;
        while (!current.empty()) {
            if (seen.count(current) != 0 || current == node.id) {
                return true;
            }
            seen.insert(current);
            const ProjectNode* next = nullptr;
            for (const ProjectNode& candidate : nodes) {
                if (candidate.id == current) {
                    next = &candidate;
                    break;
                }
            }
            if (next == nullptr) {
                break;
            }
            current = next->parent;
        }
    }
    return false;
}

Core::Result<StoredPath> ParsePathObject(const nlohmann::json& item) {
    StoredPath path;
    if (!item.is_object() || !item.contains("kind") || !item.contains("value") ||
        !item["kind"].is_string() || !item["value"].is_string()) {
        return Core::Result<StoredPath>::Fail(
            ParseError("script.path is invalid", "工程剧本路径无效"));
    }
    const std::string kind = item["kind"].get<std::string>();
    path.value = item["value"].get<std::string>();
    if (kind == "project-relative") {
        path.kind = StoredPathKind::ProjectRelative;
        if (path.value.empty() || path.value.find("..") != std::string::npos ||
            path.value.find('\\') != std::string::npos) {
            return Core::Result<StoredPath>::Fail(
                ParseError("project-relative path escapes root", "工程相对路径无效"));
        }
    } else if (kind == "absolute") {
        path.kind = StoredPathKind::Absolute;
    } else {
        return Core::Result<StoredPath>::Fail(
            ParseError("unknown path kind", "不支持的路径类型"));
    }
    return Core::Result<StoredPath>::Ok(std::move(path));
}

std::uint32_t Rotr(std::uint32_t value, std::uint32_t bits) {
    return (value >> bits) | (value << (32u - bits));
}

std::string Sha256Hex(const std::vector<std::uint8_t>& data) {
    static const std::uint32_t k[64] = {
        0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4,
        0xab1c5ed5, 0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe,
        0x9bdc06a7, 0xc19bf174, 0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f,
        0x4a7484aa, 0x5cb0a9dc, 0x76f988da, 0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7,
        0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967, 0x27b70a85, 0x2e1b2138, 0x4d2c6dfc,
        0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85, 0xa2bfe8a1, 0xa81a664b,
        0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070, 0x19a4c116,
        0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
        0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7,
        0xc67178f2};
    std::uint32_t h[8] = {0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
                          0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19};
    std::vector<std::uint8_t> msg = data;
    const std::uint64_t bitLen = static_cast<std::uint64_t>(data.size()) * 8ull;
    msg.push_back(0x80);
    while ((msg.size() % 64) != 56) {
        msg.push_back(0);
    }
    for (int i = 7; i >= 0; --i) {
        msg.push_back(static_cast<std::uint8_t>((bitLen >> (i * 8)) & 0xffu));
    }
    for (std::size_t chunk = 0; chunk < msg.size(); chunk += 64) {
        std::uint32_t w[64];
        for (int i = 0; i < 16; ++i) {
            w[i] = (static_cast<std::uint32_t>(msg[chunk + static_cast<std::size_t>(i) * 4]) << 24) |
                   (static_cast<std::uint32_t>(msg[chunk + static_cast<std::size_t>(i) * 4 + 1]) << 16) |
                   (static_cast<std::uint32_t>(msg[chunk + static_cast<std::size_t>(i) * 4 + 2]) << 8) |
                   static_cast<std::uint32_t>(msg[chunk + static_cast<std::size_t>(i) * 4 + 3]);
        }
        for (int i = 16; i < 64; ++i) {
            const std::uint32_t s0 = Rotr(w[i - 15], 7) ^ Rotr(w[i - 15], 18) ^ (w[i - 15] >> 3);
            const std::uint32_t s1 = Rotr(w[i - 2], 17) ^ Rotr(w[i - 2], 19) ^ (w[i - 2] >> 10);
            w[i] = w[i - 16] + s0 + w[i - 7] + s1;
        }
        std::uint32_t a = h[0], b = h[1], c = h[2], d = h[3], e = h[4], f = h[5], g = h[6], hh = h[7];
        for (int i = 0; i < 64; ++i) {
            const std::uint32_t s1 = Rotr(e, 6) ^ Rotr(e, 11) ^ Rotr(e, 25);
            const std::uint32_t ch = (e & f) ^ ((~e) & g);
            const std::uint32_t temp1 = hh + s1 + ch + k[i] + w[i];
            const std::uint32_t s0 = Rotr(a, 2) ^ Rotr(a, 13) ^ Rotr(a, 22);
            const std::uint32_t maj = (a & b) ^ (a & c) ^ (b & c);
            const std::uint32_t temp2 = s0 + maj;
            hh = g;
            g = f;
            f = e;
            e = d + temp1;
            d = c;
            c = b;
            b = a;
            a = temp1 + temp2;
        }
        h[0] += a;
        h[1] += b;
        h[2] += c;
        h[3] += d;
        h[4] += e;
        h[5] += f;
        h[6] += g;
        h[7] += hh;
    }
    std::ostringstream out;
    out << std::hex;
    for (std::uint32_t word : h) {
        for (int shift = 28; shift >= 0; shift -= 4) {
            out << ((word >> shift) & 0xfu);
        }
    }
    return out.str();
}

} // namespace

std::string ProjectFile::MakeProjectId() {
    const auto now = std::chrono::steady_clock::now().time_since_epoch().count();
    std::ostringstream out;
    out << "proj-" << std::hex << static_cast<std::uint64_t>(now);
    return out.str();
}

Core::Result<StoredPath> ProjectFile::MakeStoredPath(const std::string& projectDir,
                                                     const std::string& utf8Path) {
    StoredPath path;
    if (utf8Path.empty()) {
        return Core::Result<StoredPath>::Ok(path);
    }
    if (!projectDir.empty() && Platform::Paths::IsWithin(projectDir, utf8Path)) {
        auto relative = Platform::Paths::RelativeTo(projectDir, utf8Path);
        if (relative.IsOk()) {
            path.kind = StoredPathKind::ProjectRelative;
            path.value = relative.Value();
            return Core::Result<StoredPath>::Ok(std::move(path));
        }
    }
    auto canonical = Platform::Paths::WeaklyCanonical(utf8Path);
    path.kind = StoredPathKind::Absolute;
    path.value = Platform::Paths::NormalizeSlashes(canonical.IsOk() ? canonical.Value() : utf8Path);
    return Core::Result<StoredPath>::Ok(std::move(path));
}

Core::Result<std::string> ProjectFile::ResolveStoredPath(const std::string& projectDir,
                                                         const StoredPath& path) {
    if (path.value.empty()) {
        return Core::Result<std::string>::Ok(std::string{});
    }
    if (path.kind == StoredPathKind::Absolute) {
        return Platform::Paths::WeaklyCanonical(path.value);
    }
    if (projectDir.empty()) {
        return Core::Result<std::string>::Fail(
            ParseError("project directory is empty", "工程目录无效"));
    }
    const std::string joined = Platform::Paths::Join(projectDir, path.value);
    if (!Platform::Paths::IsWithin(projectDir, joined)) {
        return Core::Result<std::string>::Fail(
            ParseError("resolved path escapes project", "工程相对路径越界"));
    }
    return Platform::Paths::WeaklyCanonical(joined);
}

Core::Result<std::string> ProjectFile::Sha256File(const std::string& utf8Path) {
    auto bytes = Platform::Paths::ReadBinaryFile(utf8Path);
    if (!bytes.IsOk()) {
        return Core::Result<std::string>::Fail(bytes.GetError());
    }
    return Core::Result<std::string>::Ok(Sha256Hex(bytes.Value()));
}

Core::Result<ProjectSnapshot> ProjectFile::Parse(const std::string& jsonText,
                                                 const std::string& projectDir) {
    nlohmann::json root;
    try {
        root = nlohmann::json::parse(jsonText);
    } catch (const nlohmann::json::exception& ex) {
        return Core::Result<ProjectSnapshot>::Fail(
            ParseError(std::string("Invalid project JSON: ") + ex.what(), "工程文件不是合法 JSON"));
    }
    if (!root.is_object()) {
        return Core::Result<ProjectSnapshot>::Fail(
            ParseError("Project root is not an object", "工程文件格式无效"));
    }
    if (!root.contains("format") || !root["format"].is_string() ||
        root["format"].get<std::string>() != "DirectorDeskProject") {
        return Core::Result<ProjectSnapshot>::Fail(
            ParseError("format is not DirectorDeskProject", "不是 DirectorDesk 工程文件"));
    }
    if (!root.contains("formatVersion") || !root["formatVersion"].is_number_integer()) {
        return Core::Result<ProjectSnapshot>::Fail(
            ParseError("formatVersion is missing", "工程文件缺少版本号"));
    }
    const int version = root["formatVersion"].get<int>();
    if (version > 1) {
        return Core::Result<ProjectSnapshot>::Fail(Core::Error::Make(
            Core::ErrorCode::Unsupported, "Project formatVersion is newer than 1",
            "工程文件版本过高，请升级 DirectorDesk"));
    }
    if (version < 1) {
        return Core::Result<ProjectSnapshot>::Fail(Core::Error::Make(
            Core::ErrorCode::Unsupported, "Project formatVersion is older than 1",
            "不支持的工程文件版本"));
    }

    ProjectSnapshot snapshot;
    snapshot.projectId = root.value("projectId", ProjectFile::MakeProjectId());
    snapshot.name = root.value("name", "未命名工程");
    if (snapshot.projectId.empty()) {
        return Core::Result<ProjectSnapshot>::Fail(
            ParseError("projectId is empty", "工程 ID 无效"));
    }

    if (root.contains("script") && root["script"].is_object() && root["script"].contains("path")) {
        auto scriptPath = ParsePathObject(root["script"]["path"]);
        if (!scriptPath.IsOk()) {
            return Core::Result<ProjectSnapshot>::Fail(scriptPath.GetError());
        }
        snapshot.script = scriptPath.Value();
        auto resolved = ResolveStoredPath(projectDir, snapshot.script);
        if (!resolved.IsOk()) {
            snapshot.diagnostics.push_back("剧本路径无法解析");
        } else if (!resolved.Value().empty() && !Platform::Paths::Exists(resolved.Value())) {
            snapshot.diagnostics.push_back("剧本文件缺失");
        }
    }

    std::unordered_set<std::string> assetIds;
    if (root.contains("assets") && root["assets"].is_array()) {
        for (const nlohmann::json& item : root["assets"]) {
            if (!item.is_object() || !item.contains("refId") || !item.contains("source") ||
                !item["refId"].is_string() || !item["source"].is_string()) {
                return Core::Result<ProjectSnapshot>::Fail(
                    ParseError("asset entry is invalid", "资产引用无效"));
            }
            ProjectAssetRef asset;
            asset.refId = item["refId"].get<std::string>();
            const std::string source = item["source"].get<std::string>();
            if (assetIds.count(asset.refId) != 0 || asset.refId.empty()) {
                return Core::Result<ProjectSnapshot>::Fail(
                    ParseError("duplicate or empty asset refId", "资产引用 ID 重复"));
            }
            if (source == "project") {
                asset.source = ProjectAssetSource::Project;
                asset.path = item.value("path", "");
                asset.sha256 = item.value("sha256", "");
                if (asset.path.empty() || asset.path.find("..") != std::string::npos ||
                    asset.path.find('\\') != std::string::npos) {
                    return Core::Result<ProjectSnapshot>::Fail(
                        ParseError("project asset path is invalid", "工程内资产路径无效"));
                }
                if (!asset.sha256.empty() && !IsHexSha(asset.sha256)) {
                    return Core::Result<ProjectSnapshot>::Fail(
                        ParseError("asset sha256 is invalid", "资产校验值无效"));
                }
            } else if (source == "user-library") {
                asset.source = ProjectAssetSource::UserLibrary;
                asset.assetId = item.value("assetId", "");
                asset.sha256 = item.value("sha256", "");
                asset.path = item.value("path", "");
                if (asset.assetId.empty()) {
                    return Core::Result<ProjectSnapshot>::Fail(
                        ParseError("user-library assetId is empty", "资源库资产 ID 无效"));
                }
            } else if (source == "official") {
                asset.source = ProjectAssetSource::Official;
                asset.assetId = item.value("assetId", "");
                asset.version = item.value("version", "");
                asset.entrypoint = item.value("entrypoint", "");
                if (asset.assetId.empty() || asset.version.empty() || asset.entrypoint.empty()) {
                    return Core::Result<ProjectSnapshot>::Fail(
                        ParseError("official asset is incomplete", "官方资产引用不完整"));
                }
            } else {
                return Core::Result<ProjectSnapshot>::Fail(
                    ParseError("unknown asset source", "不支持的资产来源"));
            }
            assetIds.insert(asset.refId);
            snapshot.assets.push_back(std::move(asset));
        }
    }

    std::unordered_set<std::string> nodeIds;
    if (root.contains("scene") && root["scene"].is_object() && root["scene"].contains("nodes") &&
        root["scene"]["nodes"].is_array()) {
        for (const nlohmann::json& item : root["scene"]["nodes"]) {
            if (!item.is_object() || !item.contains("id") || !item["id"].is_string()) {
                return Core::Result<ProjectSnapshot>::Fail(
                    ParseError("scene node is invalid", "场景节点无效"));
            }
            ProjectNode node;
            node.id = item["id"].get<std::string>();
            if (node.id.empty() || nodeIds.count(node.id) != 0) {
                return Core::Result<ProjectSnapshot>::Fail(
                    ParseError("duplicate scene node id", "场景节点 ID 重复"));
            }
            node.name = item.value("name", node.id);
            node.assetRef = item.value("assetRef", "");
            if (item.contains("parent") && !item["parent"].is_null()) {
                if (!item["parent"].is_string()) {
                    return Core::Result<ProjectSnapshot>::Fail(
                        ParseError("node parent is invalid", "节点父级无效"));
                }
                node.parent = item["parent"].get<std::string>();
            }
            if (!ReadVec3(item, "transform", node.position, false) && item.contains("transform") &&
                item["transform"].is_object()) {
                const nlohmann::json& transform = item["transform"];
                if (!ReadVec3(transform, "position", node.position, false) ||
                    !ReadQuat(transform, "rotation", node.rotation) ||
                    !ReadVec3(transform, "scale", node.scale, false)) {
                    return Core::Result<ProjectSnapshot>::Fail(
                        ParseError("node transform is invalid", "节点变换数值无效"));
                }
            }
            if (node.scale[0] == 0.0f || node.scale[1] == 0.0f || node.scale[2] == 0.0f) {
                return Core::Result<ProjectSnapshot>::Fail(
                    ParseError("node scale contains zero", "节点缩放不能为 0"));
            }
            node.visible = item.value("visible", true);
            nodeIds.insert(node.id);
            snapshot.nodes.push_back(std::move(node));
        }
    }
    for (const ProjectNode& node : snapshot.nodes) {
        if (!node.parent.empty() && nodeIds.count(node.parent) == 0) {
            return Core::Result<ProjectSnapshot>::Fail(
                ParseError("node parent is missing", "节点父级不存在"));
        }
    }
    if (HasCycle(snapshot.nodes)) {
        return Core::Result<ProjectSnapshot>::Fail(
            ParseError("scene graph contains a cycle", "场景图存在循环引用"));
    }

    std::unordered_set<std::string> cameraIds;
    if (root.contains("cameras") && root["cameras"].is_array()) {
        for (const nlohmann::json& item : root["cameras"]) {
            if (!item.is_object() || !item.contains("id") || !item["id"].is_string()) {
                return Core::Result<ProjectSnapshot>::Fail(
                    ParseError("camera is invalid", "相机数据无效"));
            }
            ProjectCamera camera;
            camera.id = item["id"].get<std::string>();
            if (camera.id.empty() || cameraIds.count(camera.id) != 0) {
                return Core::Result<ProjectSnapshot>::Fail(
                    ParseError("duplicate camera id", "相机 ID 重复"));
            }
            camera.name = item.value("name", camera.id);
            camera.projection = item.value("projection", "perspective");
            if (camera.projection != "perspective") {
                return Core::Result<ProjectSnapshot>::Fail(
                    ParseError("unsupported camera projection", "不支持的相机投影"));
            }
            if (!ReadVec3(item, "position", camera.position, false) ||
                !ReadQuat(item, "rotation", camera.rotation) ||
                !ReadVec3(item, "orbitTarget", camera.orbitTarget, false)) {
                return Core::Result<ProjectSnapshot>::Fail(
                    ParseError("camera numbers are invalid", "相机数值无效"));
            }
            camera.verticalFovDegrees = item.value("verticalFovDegrees", 50.0f);
            camera.nearPlane = item.value("nearPlane", 0.1f);
            camera.farPlane = item.value("farPlane", 200.0f);
            camera.preset = item.value("preset", "");
            camera.hasOrbitNumbers = item.contains("orbitDistance") && item.contains("orbitYaw") &&
                                     item.contains("orbitPitch");
            if (camera.hasOrbitNumbers) {
                camera.orbitDistance = item.value("orbitDistance", 6.0f);
                camera.orbitYaw = item.value("orbitYaw", 40.0f);
                camera.orbitPitch = item.value("orbitPitch", 25.0f);
            }
            if (camera.nearPlane <= 0.0f || camera.farPlane <= camera.nearPlane ||
                camera.verticalFovDegrees <= 1.0f || camera.verticalFovDegrees >= 179.0f) {
                return Core::Result<ProjectSnapshot>::Fail(
                    ParseError("camera clip/fov is invalid", "相机裁剪面或视场角无效"));
            }
            cameraIds.insert(camera.id);
            snapshot.cameras.push_back(std::move(camera));
        }
    }

    if (root.contains("activeCamera") && !root["activeCamera"].is_null()) {
        if (!root["activeCamera"].is_string()) {
            return Core::Result<ProjectSnapshot>::Fail(
                ParseError("activeCamera is invalid", "当前相机无效"));
        }
        snapshot.activeCamera = root["activeCamera"].get<std::string>();
        if (!snapshot.activeCamera.empty() && cameraIds.count(snapshot.activeCamera) == 0) {
            return Core::Result<ProjectSnapshot>::Fail(
                ParseError("activeCamera is missing", "当前相机不存在"));
        }
    }

    if (root.contains("shotLinks") && root["shotLinks"].is_array()) {
        std::unordered_set<std::string> shotIds;
        for (const nlohmann::json& item : root["shotLinks"]) {
            if (!item.is_object() || !item.contains("shotId") || !item.contains("cameraId") ||
                !item["shotId"].is_string() || !item["cameraId"].is_string()) {
                return Core::Result<ProjectSnapshot>::Fail(
                    ParseError("shot link is invalid", "镜头关联无效"));
            }
            ProjectShotLink link;
            link.shotId = item["shotId"].get<std::string>();
            link.cameraId = item["cameraId"].get<std::string>();
            if (link.shotId.empty() || shotIds.count(link.shotId) != 0) {
                return Core::Result<ProjectSnapshot>::Fail(
                    ParseError("duplicate shot link", "同一镜头不能关联多台相机"));
            }
            shotIds.insert(link.shotId);
            if (cameraIds.count(link.cameraId) == 0) {
                snapshot.diagnostics.push_back("悬空镜头关联：" + link.shotId);
            }
            snapshot.shotLinks.push_back(std::move(link));
        }
    }

    if (root.contains("lighting") && root["lighting"].is_object()) {
        snapshot.lightingPreset = root["lighting"].value("preset", "neutral");
    }
    if (root.contains("storyboard") && root["storyboard"].is_object()) {
        snapshot.storyboardLayout = root["storyboard"].value("layout", "left-to-right");
        if (snapshot.storyboardLayout != "left-to-right") {
            return Core::Result<ProjectSnapshot>::Fail(
                ParseError("unsupported storyboard layout", "不支持的分镜布局"));
        }
        if (root["storyboard"].contains("collapsedScenes") &&
            root["storyboard"]["collapsedScenes"].is_array()) {
            for (const nlohmann::json& item : root["storyboard"]["collapsedScenes"]) {
                if (item.is_string() && !item.get<std::string>().empty()) {
                    snapshot.collapsedScenes.push_back(item.get<std::string>());
                }
            }
        }
    }
    return Core::Result<ProjectSnapshot>::Ok(std::move(snapshot));
}

Core::Result<std::string> ProjectFile::Serialize(const ProjectSnapshot& snapshot) {
    nlohmann::json root;
    root["format"] = "DirectorDeskProject";
    root["formatVersion"] = 1;
    root["createdBy"] = "DirectorDesk 0.1.2";
    root["projectId"] = snapshot.projectId;
    root["name"] = snapshot.name;
    if (!snapshot.script.value.empty()) {
        nlohmann::json script;
        script["path"] = {{"kind", snapshot.script.kind == StoredPathKind::ProjectRelative
                                       ? "project-relative"
                                       : "absolute"},
                          {"value", snapshot.script.value}};
        root["script"] = std::move(script);
    }

    nlohmann::json assets = nlohmann::json::array();
    for (const ProjectAssetRef& asset : snapshot.assets) {
        nlohmann::json item;
        item["refId"] = asset.refId;
        if (asset.source == ProjectAssetSource::Project) {
            item["source"] = "project";
            item["path"] = asset.path;
            item["sha256"] = asset.sha256;
        } else if (asset.source == ProjectAssetSource::Official) {
            item["source"] = "official";
            item["assetId"] = asset.assetId;
            item["version"] = asset.version;
            item["entrypoint"] = asset.entrypoint;
        } else {
            item["source"] = "user-library";
            item["assetId"] = asset.assetId;
            if (!asset.sha256.empty()) {
                item["sha256"] = asset.sha256;
            }
            if (!asset.path.empty()) {
                item["path"] = asset.path;
            }
        }
        assets.push_back(std::move(item));
    }
    root["assets"] = std::move(assets);

    nlohmann::json nodes = nlohmann::json::array();
    for (const ProjectNode& node : snapshot.nodes) {
        nlohmann::json item;
        item["id"] = node.id;
        item["name"] = node.name;
        item["assetRef"] = node.assetRef;
        item["parent"] = node.parent.empty() ? nlohmann::json(nullptr) : nlohmann::json(node.parent);
        item["transform"] = {{"position", Vec3Json(node.position)},
                             {"rotation", QuatJson(node.rotation)},
                             {"scale", Vec3Json(node.scale)}};
        item["visible"] = node.visible;
        nodes.push_back(std::move(item));
    }
    root["scene"] = {{"nodes", std::move(nodes)}};

    nlohmann::json cameras = nlohmann::json::array();
    for (const ProjectCamera& camera : snapshot.cameras) {
        nlohmann::json item;
        item["id"] = camera.id;
        item["name"] = camera.name;
        item["projection"] = camera.projection;
        item["position"] = Vec3Json(camera.position);
        item["rotation"] = QuatJson(camera.rotation);
        item["verticalFovDegrees"] = camera.verticalFovDegrees;
        item["nearPlane"] = camera.nearPlane;
        item["farPlane"] = camera.farPlane;
        item["orbitTarget"] = Vec3Json(camera.orbitTarget);
        item["orbitDistance"] = camera.orbitDistance;
        item["orbitYaw"] = camera.orbitYaw;
        item["orbitPitch"] = camera.orbitPitch;
        item["preset"] = camera.preset;
        cameras.push_back(std::move(item));
    }
    root["cameras"] = std::move(cameras);
    if (snapshot.activeCamera.empty()) {
        root["activeCamera"] = nullptr;
    } else {
        root["activeCamera"] = snapshot.activeCamera;
    }

    nlohmann::json links = nlohmann::json::array();
    for (const ProjectShotLink& link : snapshot.shotLinks) {
        links.push_back({{"shotId", link.shotId}, {"cameraId", link.cameraId}});
    }
    root["shotLinks"] = std::move(links);
    root["lighting"] = {{"preset", snapshot.lightingPreset}};
    root["storyboard"] = {{"layout", snapshot.storyboardLayout},
                          {"collapsedScenes", snapshot.collapsedScenes}};
    return Core::Result<std::string>::Ok(root.dump(2));
}

Core::Result<ProjectSnapshot> ProjectFile::Load(const std::string& utf8Path) {
    auto text = Platform::Paths::ReadTextFile(utf8Path);
    if (!text.IsOk()) {
        return Core::Result<ProjectSnapshot>::Fail(text.GetError());
    }
    return Parse(text.Value(), Platform::Paths::Parent(utf8Path));
}

Core::Result<void> ProjectFile::Save(const std::string& utf8Path, const ProjectSnapshot& snapshot) {
    auto text = Serialize(snapshot);
    if (!text.IsOk()) {
        return Core::Result<void>::Fail(text.GetError());
    }
    const std::string tempPath = utf8Path + ".ddtmp";
    auto written = Platform::Paths::WriteTextFile(tempPath, text.Value());
    if (!written.IsOk()) {
        return written;
    }
    auto verify = Load(tempPath);
    if (!verify.IsOk()) {
        Platform::Paths::RemoveFile(tempPath);
        return Core::Result<void>::Fail(verify.GetError());
    }
    auto replaced = Platform::Paths::AtomicReplace(tempPath, utf8Path);
    if (!replaced.IsOk()) {
        Platform::Paths::RemoveFile(tempPath);
        return replaced;
    }
    return Core::Result<void>::Ok();
}

} // namespace DirectorDesk::App
