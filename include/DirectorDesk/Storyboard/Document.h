// Document: Public or internal interface for the DirectorDesk Storyboard module.
// This file owns project behavior only; keep platform and dependency boundaries explicit.

#pragma once

#include "DirectorDesk/Storyboard/Layout.h"
#include "DirectorDesk/Storyboard/Types.h"

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace DirectorDesk::Storyboard {

class Document {
public:
    // Storyboard consumes immutable script/link snapshots and owns preview lifecycle state.
    void ApplySource(const StoryboardSourceSnapshot& snapshot);
    void Clear();
    void SetCollapsed(const std::string& sceneId, bool collapsed);
    void SetSelectedShot(const std::string& shotId);
    void MarkLinkedStale();
    // Camera edits invalidate only shots linked to that camera.
    void MarkCameraShotsStale(const std::string& cameraId);
    void MarkShotStale(const std::string& shotId);
    void MarkShotRendering(const std::string& shotId);
    void SetThumbnail(const std::string& shotId, ImageBuffer pixels, std::uint64_t renderRevision);
    void MarkShotFailed(const std::string& shotId);
    void MarkShotExported(const std::string& shotId);
    void Touch(const std::string& shotId, std::uint64_t frame);
    void EvictThumbnails(const std::vector<std::string>& keepIds, std::size_t maxCount,
                         std::vector<std::uint16_t>& destroyedTextures);

    [[nodiscard]] const LayoutResult& Layout() const {
        return m_layout;
    }
    [[nodiscard]] LayoutResult ExportLayout() const;
    [[nodiscard]] const std::vector<std::string>& CollapsedScenes() const {
        return m_collapsed;
    }
    [[nodiscard]] bool HeldLastValid() const {
        return m_heldLastValid;
    }
    [[nodiscard]] const std::string& SelectedShotId() const {
        return m_selectedShotId;
    }
    [[nodiscard]] const ThumbnailRecord* Thumbnail(const std::string& shotId) const;
    ThumbnailRecord* ThumbnailMutable(const std::string& shotId);
    // Picks one visible stale shot, prioritizing the currently selected shot.
    [[nodiscard]] std::string NextThumbnailShot(const ViewRect& view) const;
    [[nodiscard]] PreviewIssueCount CountExportPreviewIssues() const;
    void TakeDestroyedTextures(std::vector<std::uint16_t>& out);
    [[nodiscard]] std::uint64_t DirectorRevision() const {
        return m_directorRevision;
    }

private:
    void RebuildLayout();

    StoryboardSourceSnapshot m_source;
    LayoutResult m_layout;
    std::vector<std::string> m_collapsed;
    std::string m_selectedShotId;
    bool m_heldLastValid = false;
    std::uint64_t m_directorRevision = 1;
    std::unordered_map<std::string, ThumbnailRecord> m_thumbs;
    std::vector<std::uint16_t> m_destroyedTextures;
};

class ThumbnailScheduler {
public:
    void BeginFrame();
    void NotifyBusy(std::uint64_t nowMs);
    [[nodiscard]] bool ShouldRun(std::uint64_t nowMs) const;
    void ConsumeFrame();

    static constexpr std::uint64_t kDebounceMs = 300;

private:
    std::uint64_t m_lastBusyMs = 0;
    bool m_frameUsed = false;
};

} // namespace DirectorDesk::Storyboard
