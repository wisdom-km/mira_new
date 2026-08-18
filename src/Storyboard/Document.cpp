// Document: Implementation for the DirectorDesk Storyboard module.
// This file owns project behavior only; keep platform and dependency boundaries explicit.

#include "DirectorDesk/Storyboard/Document.h"

#include <algorithm>
#include <unordered_set>

namespace DirectorDesk::Storyboard {
namespace {

bool ContainsId(const std::vector<std::string>& ids, const std::string& id) {
    return std::find(ids.begin(), ids.end(), id) != ids.end();
}

} // namespace

void Document::RebuildLayout() {
    for (SceneSource& scene : m_source.scenes) {
        scene.collapsed = ContainsId(m_collapsed, scene.id);
    }
    m_source.selectedShotId = m_selectedShotId;
    m_layout = BuildLayout(m_source, DefaultLayoutMetrics(), false);
    for (LayoutCard& card : m_layout.cards) {
        if (card.kind != CardKind::Shot) {
            continue;
        }
        if (card.link == LinkStatus::Unlinked) {
            card.preview = PreviewStatus::Missing;
            continue;
        }
        if (const ThumbnailRecord* thumb = Thumbnail(card.shotId)) {
            card.preview = thumb->status;
            card.exported = thumb->exported;
        } else {
            card.preview = PreviewStatus::Stale;
        }
    }
}

void Document::ApplySource(const StoryboardSourceSnapshot& snapshot) {
    if (!snapshot.scriptValid) {
        m_heldLastValid = !m_source.scenes.empty();
        return;
    }
    m_heldLastValid = false;
    m_source = snapshot;
    m_directorRevision = snapshot.directorRevision;
    m_selectedShotId = snapshot.selectedShotId;
    std::unordered_set<std::string> liveScenes;
    std::unordered_set<std::string> liveShots;
    for (const SceneSource& scene : snapshot.scenes) {
        liveScenes.insert(scene.id);
        for (const ShotSource& shot : scene.shots) {
            liveShots.insert(shot.id);
        }
    }
    m_collapsed.clear();
    for (const SceneSource& scene : snapshot.scenes) {
        if (scene.collapsed && liveScenes.count(scene.id) != 0) {
            m_collapsed.push_back(scene.id);
        }
    }
    for (auto it = m_thumbs.begin(); it != m_thumbs.end();) {
        if (liveShots.count(it->first) == 0) {
            if (it->second.textureIndex != 0xFFFFu) {
                m_destroyedTextures.push_back(it->second.textureIndex);
            }
            it = m_thumbs.erase(it);
        } else {
            ++it;
        }
    }
    RebuildLayout();
}

void Document::Clear() {
    for (const auto& entry : m_thumbs) {
        if (entry.second.textureIndex != 0xFFFFu) {
            m_destroyedTextures.push_back(entry.second.textureIndex);
        }
    }
    m_thumbs.clear();
    m_source = {};
    m_layout = {};
    m_collapsed.clear();
    m_selectedShotId.clear();
    m_heldLastValid = false;
    m_directorRevision = 1;
}

void Document::SetCollapsed(const std::string& sceneId, bool collapsed) {
    if (collapsed && !ContainsId(m_collapsed, sceneId)) {
        m_collapsed.push_back(sceneId);
    }
    if (!collapsed) {
        m_collapsed.erase(std::remove(m_collapsed.begin(), m_collapsed.end(), sceneId),
                          m_collapsed.end());
    }
    RebuildLayout();
}

void Document::SetSelectedShot(const std::string& shotId) {
    m_selectedShotId = shotId;
    RebuildLayout();
}

void Document::MarkLinkedStale() {
    for (auto& entry : m_thumbs) {
        if (entry.second.status == PreviewStatus::Ready ||
            entry.second.status == PreviewStatus::Failed) {
            entry.second.status = PreviewStatus::Stale;
        }
    }
    for (const LayoutCard& card : m_layout.cards) {
        if (card.kind == CardKind::Shot && card.link == LinkStatus::Linked &&
            m_thumbs.find(card.shotId) == m_thumbs.end()) {
            ThumbnailRecord record;
            record.status = PreviewStatus::Stale;
            m_thumbs.emplace(card.shotId, record);
        }
    }
    RebuildLayout();
}

void Document::MarkCameraShotsStale(const std::string& cameraId) {
    std::vector<std::string> ids;
    for (const LayoutCard& card : m_layout.cards) {
        if (card.kind == CardKind::Shot && card.cameraId == cameraId) {
            ids.push_back(card.shotId);
        }
    }
    for (const std::string& id : ids) {
        m_thumbs[id].status = PreviewStatus::Stale;
    }
    RebuildLayout();
}

void Document::MarkShotStale(const std::string& shotId) {
    ThumbnailRecord& record = m_thumbs[shotId];
    if (record.status != PreviewStatus::Rendering) {
        record.status = PreviewStatus::Stale;
    }
    RebuildLayout();
}

void Document::MarkShotRendering(const std::string& shotId) {
    m_thumbs[shotId].status = PreviewStatus::Rendering;
    RebuildLayout();
}

void Document::SetThumbnail(const std::string& shotId, ImageBuffer pixels,
                            std::uint64_t renderRevision) {
    ThumbnailRecord& record = m_thumbs[shotId];
    record.pixels = std::move(pixels);
    record.renderRevision = renderRevision;
    record.status = PreviewStatus::Ready;
    RebuildLayout();
}

void Document::MarkShotFailed(const std::string& shotId) {
    m_thumbs[shotId].status = PreviewStatus::Failed;
    RebuildLayout();
}

void Document::MarkShotExported(const std::string& shotId) {
    m_thumbs[shotId].exported = ExportStatus::Exported;
    RebuildLayout();
}

void Document::Touch(const std::string& shotId, std::uint64_t frame) {
    if (ThumbnailRecord* record = ThumbnailMutable(shotId)) {
        record->lastUsedFrame = frame;
    }
}

void Document::EvictThumbnails(const std::vector<std::string>& keepIds, std::size_t maxCount,
                               std::vector<std::uint16_t>& destroyedTextures) {
    if (m_thumbs.size() <= maxCount) {
        return;
    }
    std::vector<std::pair<std::uint64_t, std::string>> ranked;
    for (const auto& entry : m_thumbs) {
        if (std::find(keepIds.begin(), keepIds.end(), entry.first) != keepIds.end()) {
            continue;
        }
        ranked.push_back({entry.second.lastUsedFrame, entry.first});
    }
    std::sort(ranked.begin(), ranked.end());
    std::size_t removable = m_thumbs.size() > maxCount ? m_thumbs.size() - maxCount : 0;
    for (std::size_t i = 0; i < ranked.size() && removable > 0; ++i, --removable) {
        auto found = m_thumbs.find(ranked[i].second);
        if (found == m_thumbs.end()) {
            continue;
        }
        if (found->second.textureIndex != 0xFFFFu) {
            destroyedTextures.push_back(found->second.textureIndex);
        }
        m_thumbs.erase(found);
    }
}

LayoutResult Document::ExportLayout() const {
    LayoutResult layout = BuildLayout(m_source, DefaultLayoutMetrics(), true);
    for (LayoutCard& card : layout.cards) {
        if (card.kind != CardKind::Shot) {
            continue;
        }
        if (card.link == LinkStatus::Unlinked) {
            card.preview = PreviewStatus::Missing;
            continue;
        }
        if (const ThumbnailRecord* thumb = Thumbnail(card.shotId)) {
            card.preview = thumb->status;
            card.exported = thumb->exported;
        } else {
            card.preview = PreviewStatus::Stale;
        }
    }
    return layout;
}

const ThumbnailRecord* Document::Thumbnail(const std::string& shotId) const {
    const auto found = m_thumbs.find(shotId);
    return found == m_thumbs.end() ? nullptr : &found->second;
}

ThumbnailRecord* Document::ThumbnailMutable(const std::string& shotId) {
    const auto found = m_thumbs.find(shotId);
    return found == m_thumbs.end() ? nullptr : &found->second;
}

std::string Document::NextThumbnailShot(const ViewRect& view) const {
    const auto visible = VisibleCardIds(m_layout, view);
    auto pick = [&](PreviewStatus status, bool selectedOnly) -> std::string {
        for (const LayoutCard& card : m_layout.cards) {
            if (card.kind != CardKind::Shot || card.link != LinkStatus::Linked) {
                continue;
            }
            if (selectedOnly && card.shotId != m_selectedShotId) {
                continue;
            }
            const bool isVisible =
                std::find(visible.begin(), visible.end(), card.id) != visible.end();
            if (!selectedOnly && !isVisible) {
                continue;
            }
            const ThumbnailRecord* thumb = Thumbnail(card.shotId);
            const PreviewStatus current = thumb == nullptr ? PreviewStatus::Stale : thumb->status;
            if (current == status) {
                return card.shotId;
            }
        }
        return {};
    };
    std::string id = pick(PreviewStatus::Stale, true);
    if (id.empty()) {
        id = pick(PreviewStatus::Missing, true);
    }
    if (id.empty()) {
        id = pick(PreviewStatus::Stale, false);
    }
    if (id.empty()) {
        id = pick(PreviewStatus::Failed, true);
    }
    return id;
}

PreviewIssueCount Document::CountExportPreviewIssues() const {
    PreviewIssueCount count;
    const LayoutResult layout = ExportLayout();
    for (const LayoutCard& card : layout.cards) {
        if (card.kind != CardKind::Shot) {
            continue;
        }
        switch (card.preview) {
        case PreviewStatus::Stale:
            ++count.stale;
            break;
        case PreviewStatus::Failed:
            ++count.failed;
            break;
        case PreviewStatus::Missing:
            ++count.missing;
            break;
        default:
            break;
        }
    }
    return count;
}

void Document::TakeDestroyedTextures(std::vector<std::uint16_t>& out) {
    out.insert(out.end(), m_destroyedTextures.begin(), m_destroyedTextures.end());
    m_destroyedTextures.clear();
}

void ThumbnailScheduler::BeginFrame() {
    m_frameUsed = false;
}

void ThumbnailScheduler::NotifyBusy(std::uint64_t nowMs) {
    m_lastBusyMs = nowMs;
}

bool ThumbnailScheduler::ShouldRun(std::uint64_t nowMs) const {
    if (m_frameUsed) {
        return false;
    }
    return nowMs >= m_lastBusyMs + kDebounceMs;
}

void ThumbnailScheduler::ConsumeFrame() {
    m_frameUsed = true;
}

} // namespace DirectorDesk::Storyboard
