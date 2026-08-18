// OfficialCatalog: Implementation for the DirectorDesk Asset module.
// This file owns project behavior only; keep platform and dependency boundaries explicit.

#include "DirectorDesk/Asset/OfficialCatalog.h"

#include "DirectorDesk/Core/Error.h"
#include "DirectorDesk/Core/Sha256.h"
#include "DirectorDesk/Platform/Paths.h"

#include <algorithm>

namespace DirectorDesk::Asset {
namespace {

bool Cancelled(const std::atomic<bool>* cancel) {
    return cancel != nullptr && cancel->load();
}

Core::Error Fail(OfficialFailureKind kind, const std::string& technical) {
    return Core::Error::Make(Core::ErrorCode::IoFailure, technical,
                             OfficialCatalog::FailureMessage(kind));
}

std::string VersionDir(const std::string& cacheRoot, const ManifestAsset& asset) {
    return Platform::Paths::Join(
        Platform::Paths::Join(Platform::Paths::Join(cacheRoot, "official"), asset.id),
        asset.version);
}

} // namespace

OfficialCatalog::OfficialCatalog(std::string cacheRoot, OfficialEndpoints endpoints)
    : m_cacheRoot(std::move(cacheRoot)), m_endpoints(std::move(endpoints)) {}

void OfficialCatalog::SetAvailableBytesOverride(std::uint64_t bytes) {
    m_hasBytesOverride = true;
    m_bytesOverride = bytes;
}

void OfficialCatalog::ClearAvailableBytesOverride() {
    m_hasBytesOverride = false;
}

bool OfficialCatalog::IsConfigured() const {
    return m_endpoints.manifestUrl.rfind("https://", 0) == 0 &&
           m_endpoints.assetBaseUrl.rfind("https://", 0) == 0;
}

const char* OfficialCatalog::StatusId(OfficialDownloadStatus status) {
    switch (status) {
    case OfficialDownloadStatus::Queued:
        return "queued";
    case OfficialDownloadStatus::Downloading:
        return "downloading";
    case OfficialDownloadStatus::Verifying:
        return "verifying";
    case OfficialDownloadStatus::Ready:
        return "ready";
    case OfficialDownloadStatus::Failed:
        return "failed";
    case OfficialDownloadStatus::Cancelled:
        return "cancelled";
    case OfficialDownloadStatus::NotDownloaded:
    default:
        return "not-downloaded";
    }
}

const char* OfficialCatalog::FailureMessage(OfficialFailureKind kind) {
    switch (kind) {
    case OfficialFailureKind::NetworkUnreachable:
        return "网络不可达";
    case OfficialFailureKind::Timeout:
        return "下载超时";
    case OfficialFailureKind::HttpError:
        return "HTTP 错误";
    case OfficialFailureKind::DiskFull:
        return "磁盘空间不足";
    case OfficialFailureKind::WriteFailed:
        return "写入失败";
    case OfficialFailureKind::SizeMismatch:
        return "文件大小不符";
    case OfficialFailureKind::HashMismatch:
        return "校验失败";
    case OfficialFailureKind::InvalidManifest:
        return "清单非法";
    case OfficialFailureKind::Cancelled:
        return "已取消";
    case OfficialFailureKind::None:
    default:
        return "";
    }
}

const OfficialAssetState* OfficialCatalog::State(const std::string& assetId) const {
    const auto found = m_states.find(assetId);
    return found == m_states.end() ? nullptr : &found->second;
}

OfficialAssetState* OfficialCatalog::MutableState(const std::string& assetId) {
    return &m_states[assetId];
}

const ManifestAsset* OfficialCatalog::FindAsset(const std::string& assetId) const {
    for (const ManifestAsset& asset : m_manifest.assets) {
        if (asset.id == assetId) {
            return &asset;
        }
    }
    return nullptr;
}

std::vector<ManifestAsset> OfficialCatalog::Query(const std::string& search,
                                                  const std::string& categoryId) const {
    std::vector<ManifestAsset> result;
    for (const ManifestAsset& asset : m_manifest.assets) {
        if (!categoryId.empty() && asset.category != categoryId) {
            continue;
        }
        const std::string name = PickLocale(asset.name);
        const std::string description = PickLocale(asset.description);
        bool hit = search.empty() || name.find(search) != std::string::npos ||
                   description.find(search) != std::string::npos;
        for (const std::string& tag : asset.tags) {
            hit = hit || tag.find(search) != std::string::npos;
        }
        if (hit) {
            result.push_back(asset);
        }
    }
    return result;
}

void OfficialCatalog::DiscoverReadyAssets() {
    for (const ManifestAsset& asset : m_manifest.assets) {
        OfficialAssetState& state = m_states[asset.id];
        const std::string root = VersionDir(m_cacheRoot, asset);
        const std::string entry = Platform::Paths::Join(root, asset.entrypoint);
        if (!Platform::Paths::Exists(entry)) {
            if (state.status == OfficialDownloadStatus::Ready) {
                state.status = OfficialDownloadStatus::NotDownloaded;
            }
            continue;
        }
        bool complete = true;
        for (const ManifestFile& file : asset.files) {
            if (!Platform::Paths::Exists(Platform::Paths::Join(root, file.path))) {
                complete = false;
                break;
            }
        }
        if (!complete) {
            continue;
        }
        state.status = OfficialDownloadStatus::Ready;
        state.failure = OfficialFailureKind::None;
        state.progress = 1.0f;
        state.entrypointPath = entry;
        if (!asset.preview.empty()) {
            state.previewPath = Platform::Paths::Join(root, asset.preview);
        }
    }
}

Core::Result<void> OfficialCatalog::PersistManifest(const std::string& jsonText) {
    const std::string manifests = Platform::Paths::Join(m_cacheRoot, "manifests");
    auto created = Platform::Paths::CreateDirectories(manifests);
    if (!created.IsOk()) {
        return created;
    }
    const std::string current = Platform::Paths::Join(manifests, "current.json");
    const std::string previous = Platform::Paths::Join(manifests, "previous.json");
    const std::string temp = Platform::Paths::Join(manifests, "current.json.part");
    if (Platform::Paths::Exists(current)) {
        auto copied = Platform::Paths::CopyFileUtf8(current, previous);
        if (!copied.IsOk()) {
            return copied;
        }
    }
    auto written = Platform::Paths::WriteTextFile(temp, jsonText);
    if (!written.IsOk()) {
        return written;
    }
    return Platform::Paths::AtomicReplace(temp, current);
}

Core::Result<void> OfficialCatalog::LoadCache() {
    const std::string manifests = Platform::Paths::Join(m_cacheRoot, "manifests");
    const std::string current = Platform::Paths::Join(manifests, "current.json");
    const std::string previous = Platform::Paths::Join(manifests, "previous.json");
    const std::string* path = nullptr;
    if (Platform::Paths::Exists(current)) {
        path = &current;
    } else if (Platform::Paths::Exists(previous)) {
        path = &previous;
    }
    if (path == nullptr) {
        return Core::Result<void>::Ok();
    }
    auto text = Platform::Paths::ReadTextFile(*path);
    if (!text.IsOk()) {
        return Core::Result<void>::Fail(text.GetError());
    }
    ManifestParseResult parsed = ParseManifest(text.Value());
    if (!parsed.accepted) {
        return Core::Result<void>::Fail(
            Fail(OfficialFailureKind::InvalidManifest, "cached manifest rejected"));
    }
    m_manifest = std::move(parsed);
    m_usedCache = true;
    DiscoverReadyAssets();
    return Core::Result<void>::Ok();
}

Core::Result<void> OfficialCatalog::ApplyManifestJson(const std::string& jsonText) {
    ManifestParseResult parsed = ParseManifest(jsonText);
    if (!parsed.accepted) {
        return Core::Result<void>::Fail(
            Fail(OfficialFailureKind::InvalidManifest, "downloaded manifest rejected"));
    }
    auto saved = PersistManifest(jsonText);
    if (!saved.IsOk()) {
        return saved;
    }
    m_manifest = std::move(parsed);
    m_usedCache = false;
    DiscoverReadyAssets();
    return Core::Result<void>::Ok();
}

Core::Result<void> OfficialCatalog::Refresh(Platform::IHttpClient& http,
                                           const std::atomic<bool>* cancel) {
    if (!IsConfigured()) {
        auto cached = LoadCache();
        m_usedCache = cached.IsOk() && !m_manifest.assets.empty();
        return cached;
    }
    if (Cancelled(cancel)) {
        return Core::Result<void>::Fail(Fail(OfficialFailureKind::Cancelled, "refresh cancelled"));
    }
    Platform::HttpGetRequest request;
    request.url = m_endpoints.manifestUrl;
    request.cancel = cancel;
    auto downloaded = http.Get(request);
    if (!downloaded.IsOk()) {
        LoadCache();
        return Core::Result<void>::Fail(downloaded.GetError());
    }
    if (downloaded.Value().status != 200) {
        LoadCache();
        return Core::Result<void>::Fail(
            Fail(OfficialFailureKind::HttpError,
                 "manifest HTTP " + std::to_string(downloaded.Value().status)));
    }
    const std::string json(downloaded.Value().body.begin(), downloaded.Value().body.end());
    auto applied = ApplyManifestJson(json);
    if (!applied.IsOk()) {
        LoadCache();
        return applied;
    }
    return Core::Result<void>::Ok();
}

Core::Result<void> FetchOfficialFile(
    const OfficialEndpoints& endpoints, const ManifestFile& file, const std::string& destPath,
    Platform::IHttpClient& http, const std::atomic<bool>* cancel,
    const std::function<void(std::uint64_t, std::uint64_t)>& progress) {
    auto url = JoinOfficialUrl(endpoints.assetBaseUrl, file.url);
    if (!url.IsOk()) {
        return Core::Result<void>::Fail(url.GetError());
    }
    auto parent = Platform::Paths::CreateDirectories(Platform::Paths::Parent(destPath));
    if (!parent.IsOk()) {
        return parent;
    }
    Platform::HttpGetRequest request;
    request.url = url.Value();
    request.outputPath = destPath;
    request.cancel = cancel;
    request.progress = progress;
    auto downloaded = http.Get(request);
    if (Cancelled(cancel)) {
        Platform::Paths::RemoveFile(destPath);
        return Core::Result<void>::Fail(Fail(OfficialFailureKind::Cancelled, "download cancelled"));
    }
    if (!downloaded.IsOk()) {
        Platform::Paths::RemoveFile(destPath);
        return Core::Result<void>::Fail(downloaded.GetError());
    }
    if (downloaded.Value().status != 200) {
        Platform::Paths::RemoveFile(destPath);
        return Core::Result<void>::Fail(
            Fail(OfficialFailureKind::HttpError,
                 "file HTTP " + std::to_string(downloaded.Value().status)));
    }
    auto bytes = Platform::Paths::ReadBinaryFile(destPath);
    if (!bytes.IsOk()) {
        Platform::Paths::RemoveFile(destPath);
        return Core::Result<void>::Fail(Fail(OfficialFailureKind::WriteFailed, "read temp failed"));
    }
    if (bytes.Value().size() != file.size) {
        Platform::Paths::RemoveFile(destPath);
        return Core::Result<void>::Fail(
            Fail(OfficialFailureKind::SizeMismatch, "size " + std::to_string(bytes.Value().size())));
    }
    if (Core::Sha256Hex(bytes.Value()) != file.sha256) {
        Platform::Paths::RemoveFile(destPath);
        return Core::Result<void>::Fail(Fail(OfficialFailureKind::HashMismatch, "sha256 mismatch"));
    }
    return Core::Result<void>::Ok();
}

OfficialFailureKind FailureFromMessage(const std::string& message) {
    if (message == OfficialCatalog::FailureMessage(OfficialFailureKind::Cancelled)) {
        return OfficialFailureKind::Cancelled;
    }
    if (message == OfficialCatalog::FailureMessage(OfficialFailureKind::HashMismatch)) {
        return OfficialFailureKind::HashMismatch;
    }
    if (message == OfficialCatalog::FailureMessage(OfficialFailureKind::SizeMismatch)) {
        return OfficialFailureKind::SizeMismatch;
    }
    if (message == OfficialCatalog::FailureMessage(OfficialFailureKind::HttpError)) {
        return OfficialFailureKind::HttpError;
    }
    if (message == OfficialCatalog::FailureMessage(OfficialFailureKind::DiskFull)) {
        return OfficialFailureKind::DiskFull;
    }
    return OfficialFailureKind::WriteFailed;
}

Core::Result<OfficialAssetState> DownloadOfficialFiles(
    const std::string& cacheRoot, const OfficialEndpoints& endpoints, const ManifestAsset& asset,
    Platform::IHttpClient& http, const std::atomic<bool>* cancel,
    const std::function<void(float)>& progress) {
    OfficialAssetState state;
    std::uint64_t needed = 0;
    for (const ManifestFile& file : asset.files) {
        needed += file.size;
    }
    auto tempDir = Platform::Paths::CreateDirectories(Platform::Paths::Join(cacheRoot, "temp"));
    if (!tempDir.IsOk()) {
        state.status = OfficialDownloadStatus::Failed;
        state.failure = OfficialFailureKind::WriteFailed;
        return Core::Result<OfficialAssetState>::Fail(tempDir.GetError());
    }
    state.status = OfficialDownloadStatus::Downloading;
    const std::string versionRoot = VersionDir(cacheRoot, asset);
    std::uint64_t done = 0;
    for (const ManifestFile& file : asset.files) {
        if (Cancelled(cancel)) {
            state.status = OfficialDownloadStatus::Cancelled;
            state.failure = OfficialFailureKind::Cancelled;
            return Core::Result<OfficialAssetState>::Fail(
                Fail(OfficialFailureKind::Cancelled, "cancelled"));
        }
        const std::string part = Platform::Paths::Join(
            Platform::Paths::Join(cacheRoot, "temp"),
            asset.id + "-" + asset.version + "-" + Platform::Paths::FileName(file.path) + ".part");
        auto downloaded = FetchOfficialFile(
            endpoints, file, part, http, cancel, [&](std::uint64_t got, std::uint64_t) {
                if (progress) {
                    progress(needed == 0
                                 ? 1.0f
                                 : static_cast<float>(done + got) / static_cast<float>(needed));
                }
            });
        if (!downloaded.IsOk()) {
            state.failure = FailureFromMessage(downloaded.GetError().userMessage);
            state.status = state.failure == OfficialFailureKind::Cancelled
                               ? OfficialDownloadStatus::Cancelled
                               : OfficialDownloadStatus::Failed;
            state.message = downloaded.GetError().userMessage;
            return Core::Result<OfficialAssetState>::Fail(downloaded.GetError());
        }
        state.status = OfficialDownloadStatus::Verifying;
        const std::string dest = Platform::Paths::Join(versionRoot, file.path);
        auto destDir = Platform::Paths::CreateDirectories(Platform::Paths::Parent(dest));
        if (!destDir.IsOk()) {
            Platform::Paths::RemoveFile(part);
            state.status = OfficialDownloadStatus::Failed;
            state.failure = OfficialFailureKind::WriteFailed;
            return Core::Result<OfficialAssetState>::Fail(destDir.GetError());
        }
        auto moved = Platform::Paths::AtomicReplace(part, dest);
        if (!moved.IsOk()) {
            Platform::Paths::RemoveFile(part);
            state.status = OfficialDownloadStatus::Failed;
            state.failure = OfficialFailureKind::WriteFailed;
            return Core::Result<OfficialAssetState>::Fail(moved.GetError());
        }
        done += file.size;
        state.progress = needed == 0 ? 1.0f : static_cast<float>(done) / static_cast<float>(needed);
    }
    state.status = OfficialDownloadStatus::Ready;
    state.failure = OfficialFailureKind::None;
    state.progress = 1.0f;
    state.entrypointPath = Platform::Paths::Join(versionRoot, asset.entrypoint);
    if (!asset.preview.empty()) {
        state.previewPath = Platform::Paths::Join(versionRoot, asset.preview);
    }
    return Core::Result<OfficialAssetState>::Ok(std::move(state));
}

Core::Result<void> OfficialCatalog::Download(const std::string& assetId, Platform::IHttpClient& http,
                                            const std::atomic<bool>* cancel,
                                            const std::function<void(float)>& progress) {
    const ManifestAsset* asset = FindAsset(assetId);
    OfficialAssetState& state = m_states[assetId];
    if (asset == nullptr) {
        state.status = OfficialDownloadStatus::Failed;
        state.failure = OfficialFailureKind::InvalidManifest;
        return Core::Result<void>::Fail(
            Fail(OfficialFailureKind::InvalidManifest, "unknown official asset"));
    }
    std::uint64_t needed = 0;
    for (const ManifestFile& file : asset->files) {
        needed += file.size;
    }
    if (m_hasBytesOverride && m_bytesOverride < needed) {
        state.status = OfficialDownloadStatus::Failed;
        state.failure = OfficialFailureKind::DiskFull;
        return Core::Result<void>::Fail(Fail(OfficialFailureKind::DiskFull, "not enough disk"));
    }
    auto downloaded =
        DownloadOfficialFiles(m_cacheRoot, m_endpoints, *asset, http, cancel, progress);
    if (!downloaded.IsOk()) {
        if (downloaded.GetError().userMessage == FailureMessage(OfficialFailureKind::Cancelled)) {
            state.status = OfficialDownloadStatus::Cancelled;
            state.failure = OfficialFailureKind::Cancelled;
        } else {
            state.status = OfficialDownloadStatus::Failed;
            state.failure = FailureFromMessage(downloaded.GetError().userMessage);
        }
        state.message = downloaded.GetError().userMessage;
        return Core::Result<void>::Fail(downloaded.GetError());
    }
    state = downloaded.Value();
    return Core::Result<void>::Ok();
}

} // namespace DirectorDesk::Asset
