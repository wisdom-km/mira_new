#pragma once

#include "DirectorDesk/Asset/Manifest.h"
#include "DirectorDesk/Core/Result.h"
#include "DirectorDesk/Platform/IHttpClient.h"

#include <atomic>
#include <cstdint>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

namespace DirectorDesk::Asset {

enum class OfficialDownloadStatus {
    NotDownloaded,
    Queued,
    Downloading,
    Verifying,
    Ready,
    Failed,
    Cancelled,
};

enum class OfficialFailureKind {
    None,
    NetworkUnreachable,
    Timeout,
    HttpError,
    DiskFull,
    WriteFailed,
    SizeMismatch,
    HashMismatch,
    InvalidManifest,
    Cancelled,
};

struct OfficialEndpoints {
    std::string manifestUrl;
    std::string assetBaseUrl;
};

struct OfficialAssetState {
    OfficialDownloadStatus status = OfficialDownloadStatus::NotDownloaded;
    OfficialFailureKind failure = OfficialFailureKind::None;
    std::string message;
    float progress = 0.0f;
    std::string entrypointPath;
    std::string previewPath;
};

Core::Result<OfficialAssetState> DownloadOfficialFiles(
    const std::string& cacheRoot, const OfficialEndpoints& endpoints, const ManifestAsset& asset,
    Platform::IHttpClient& http, const std::atomic<bool>* cancel,
    const std::function<void(float)>& progress);

class OfficialCatalog {
public:
    OfficialCatalog(std::string cacheRoot, OfficialEndpoints endpoints);

    void SetAvailableBytesOverride(std::uint64_t bytes);
    void ClearAvailableBytesOverride();

    Core::Result<void> LoadCache();
    Core::Result<void> ApplyManifestJson(const std::string& jsonText);
    Core::Result<void> Refresh(Platform::IHttpClient& http, const std::atomic<bool>* cancel);
    Core::Result<void> Download(const std::string& assetId, Platform::IHttpClient& http,
                                const std::atomic<bool>* cancel,
                                const std::function<void(float)>& progress);

    [[nodiscard]] bool IsConfigured() const;
    [[nodiscard]] bool UsedCachedManifest() const {
        return m_usedCache;
    }
    [[nodiscard]] const ManifestParseResult& Manifest() const {
        return m_manifest;
    }
    [[nodiscard]] const OfficialAssetState* State(const std::string& assetId) const;
    OfficialAssetState* MutableState(const std::string& assetId);
    [[nodiscard]] std::vector<ManifestAsset> Query(const std::string& search,
                                                   const std::string& categoryId) const;
    [[nodiscard]] const ManifestAsset* FindAsset(const std::string& assetId) const;
    [[nodiscard]] static const char* StatusId(OfficialDownloadStatus status);
    [[nodiscard]] static const char* FailureMessage(OfficialFailureKind kind);

private:
    void DiscoverReadyAssets();
    Core::Result<void> PersistManifest(const std::string& jsonText);

    std::string m_cacheRoot;
    OfficialEndpoints m_endpoints;
    ManifestParseResult m_manifest;
    std::unordered_map<std::string, OfficialAssetState> m_states;
    bool m_usedCache = false;
    bool m_hasBytesOverride = false;
    std::uint64_t m_bytesOverride = 0;
};

} // namespace DirectorDesk::Asset
