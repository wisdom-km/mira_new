#include "DirectorDesk/Storyboard/Layout.h"

#include <algorithm>
#include <cmath>

namespace DirectorDesk::Storyboard {
namespace {

bool Intersects(const LayoutCard& card, const ViewRect& view) {
    return card.x < view.x + view.w && card.x + card.w > view.x && card.y < view.y + view.h &&
           card.y + card.h > view.y;
}

bool Overlap(const LayoutCard& a, const LayoutCard& b) {
    return a.x < b.x + b.w && a.x + a.w > b.x && a.y < b.y + b.h && a.y + a.h > b.y;
}

} // namespace

const LayoutMetrics& DefaultLayoutMetrics() {
    static const LayoutMetrics metrics;
    return metrics;
}

LayoutResult BuildLayout(const StoryboardSourceSnapshot& snapshot, const LayoutMetrics& metrics,
                         bool expandAll) {
    LayoutResult result;
    LayoutCard root;
    root.kind = CardKind::Root;
    root.id = "storyboard-root";
    root.title = snapshot.documentTitle.empty() ? "剧本" : snapshot.documentTitle;
    root.x = metrics.pad;
    root.y = metrics.pad;
    root.w = metrics.rootW;
    root.h = metrics.rootH;
    result.cards.push_back(root);

    const float sceneX = metrics.pad + metrics.rootW + metrics.columnGap;
    const float shotX = sceneX + metrics.sceneW + metrics.columnGap;
    float y = metrics.pad;
    int sceneOrder = 1;
    for (const SceneSource& scene : snapshot.scenes) {
        const bool collapsed = expandAll ? false : scene.collapsed;
        LayoutCard sceneCard;
        sceneCard.kind = CardKind::Scene;
        sceneCard.id = scene.id;
        sceneCard.sceneId = scene.id;
        sceneCard.title = scene.title;
        sceneCard.order = sceneOrder++;
        sceneCard.shotCount = static_cast<int>(scene.shots.size());
        sceneCard.diagnosticCount = scene.diagnosticCount;
        sceneCard.collapsed = collapsed;
        sceneCard.x = sceneX;
        sceneCard.y = y;
        sceneCard.w = metrics.sceneW;
        sceneCard.h = metrics.sceneH;
        result.edges.push_back(LayoutEdge{root.id, scene.id});

        if (!collapsed) {
            float shotY = y;
            int shotOrder = 1;
            for (const ShotSource& shot : scene.shots) {
                LayoutCard shotCard;
                shotCard.kind = CardKind::Shot;
                shotCard.id = shot.id;
                shotCard.sceneId = scene.id;
                shotCard.shotId = shot.id;
                shotCard.cameraId = shot.cameraId;
                shotCard.title = shot.title;
                shotCard.order = shotOrder++;
                shotCard.x = shotX;
                shotCard.y = shotY;
                shotCard.w = metrics.shotW;
                shotCard.h = metrics.shotH;
                shotCard.selected = shot.id == snapshot.selectedShotId;
                shotCard.link = shot.cameraId.empty() ? LinkStatus::Unlinked : LinkStatus::Linked;
                if (!shot.cameraId.empty() && !shot.cameraExists) {
                    shotCard.preview = PreviewStatus::Failed;
                }
                result.edges.push_back(LayoutEdge{scene.id, shot.id});
                result.cards.push_back(shotCard);
                shotY += metrics.shotH + metrics.rowGap;
            }
            if (!scene.shots.empty()) {
                sceneCard.y = y;
                y = shotY;
            } else {
                y += metrics.sceneH + metrics.rowGap;
            }
        } else {
            y += metrics.sceneH + metrics.rowGap;
        }
        result.cards.push_back(std::move(sceneCard));
    }

    for (const LayoutCard& card : result.cards) {
        result.contentWidth = std::max(result.contentWidth, card.x + card.w + metrics.pad);
        result.contentHeight = std::max(result.contentHeight, card.y + card.h + metrics.pad);
    }
    return result;
}

bool CardsOverlap(const LayoutResult& layout) {
    for (std::size_t i = 0; i < layout.cards.size(); ++i) {
        for (std::size_t j = i + 1; j < layout.cards.size(); ++j) {
            if (Overlap(layout.cards[i], layout.cards[j])) {
                return true;
            }
        }
    }
    return false;
}

std::vector<std::string> VisibleCardIds(const LayoutResult& layout, const ViewRect& view) {
    std::vector<std::string> ids;
    for (const LayoutCard& card : layout.cards) {
        if (Intersects(card, view)) {
            ids.push_back(card.id);
        }
    }
    return ids;
}

const LayoutCard* FindCard(const LayoutResult& layout, const std::string& id) {
    for (const LayoutCard& card : layout.cards) {
        if (card.id == id) {
            return &card;
        }
    }
    return nullptr;
}

void FitView(const LayoutResult& layout, float viewportW, float viewportH, float& panX, float& panY,
             float& zoom, const LayoutMetrics& metrics) {
    const float width = std::max(layout.contentWidth, 1.0f);
    const float height = std::max(layout.contentHeight, 1.0f);
    const float scale = std::min(viewportW / width, viewportH / height);
    zoom = std::clamp(scale, metrics.minZoom, metrics.maxZoom);
    panX = (viewportW - width * zoom) * 0.5f;
    panY = (viewportH - height * zoom) * 0.5f;
}

void FocusCard(const LayoutResult& layout, const std::string& id, float viewportW, float viewportH,
               float& panX, float& panY, float zoom) {
    const LayoutCard* card = FindCard(layout, id);
    if (card == nullptr) {
        return;
    }
    const float cx = card->x + card->w * 0.5f;
    const float cy = card->y + card->h * 0.5f;
    panX = viewportW * 0.5f - cx * zoom;
    panY = viewportH * 0.5f - cy * zoom;
}

} // namespace DirectorDesk::Storyboard
