// Types: Public or internal interface for the DirectorDesk Storyboard module.
// This file owns project behavior only; keep platform and dependency boundaries explicit.

#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace DirectorDesk::Storyboard {

enum class CardKind {
    Root,
    Scene,
    Shot,
};

enum class LinkStatus {
    Unlinked,
    Linked,
};

enum class PreviewStatus {
    Missing,
    Stale,
    Rendering,
    Ready,
    Failed,
};

enum class ExportStatus {
    NotExported,
    Exported,
};

struct ShotSource {
    std::string id;
    std::string title;
    std::string cameraId;
    bool cameraExists = false;
    int indexInScene = 1;
};

struct SceneSource {
    std::string id;
    std::string title;
    int index = 1;
    int diagnosticCount = 0;
    bool collapsed = false;
    std::vector<ShotSource> shots;
};

struct StoryboardSourceSnapshot {
    std::string documentTitle = "剧本";
    std::string projectId;
    std::string selectedShotId;
    bool scriptValid = true;
    std::uint64_t structureRevision = 1;
    std::uint64_t directorRevision = 1;
    std::vector<SceneSource> scenes;
};

struct LayoutCard {
    CardKind kind = CardKind::Shot;
    std::string id;
    std::string title;
    std::string sceneId;
    std::string shotId;
    std::string cameraId;
    float x = 0.0f;
    float y = 0.0f;
    float w = 0.0f;
    float h = 0.0f;
    int order = 0;
    int shotCount = 0;
    int diagnosticCount = 0;
    bool collapsed = false;
    bool selected = false;
    LinkStatus link = LinkStatus::Unlinked;
    PreviewStatus preview = PreviewStatus::Missing;
    ExportStatus exported = ExportStatus::NotExported;
};

struct LayoutEdge {
    std::string fromId;
    std::string toId;
};

struct LayoutResult {
    std::vector<LayoutCard> cards;
    std::vector<LayoutEdge> edges;
    float contentWidth = 0.0f;
    float contentHeight = 0.0f;
};

struct ViewRect {
    float x = 0.0f;
    float y = 0.0f;
    float w = 0.0f;
    float h = 0.0f;
};

struct ImageBuffer {
    std::uint32_t width = 0;
    std::uint32_t height = 0;
    std::vector<std::uint8_t> rgba;
};

struct ThumbnailRecord {
    PreviewStatus status = PreviewStatus::Missing;
    ExportStatus exported = ExportStatus::NotExported;
    std::uint64_t renderRevision = 0;
    ImageBuffer pixels;
    std::uint16_t textureIndex = 0xFFFFu;
    std::uint64_t lastUsedFrame = 0;
};

struct PreviewIssueCount {
    int missing = 0;
    int stale = 0;
    int failed = 0;

    [[nodiscard]] int Total() const {
        return missing + stale + failed;
    }
};

} // namespace DirectorDesk::Storyboard
