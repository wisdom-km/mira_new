#include "DirectorDesk/Script/Parser.h"
#include "DirectorDesk/Storyboard/BoardComposer.h"
#include "DirectorDesk/Storyboard/Document.h"
#include "DirectorDesk/Storyboard/Layout.h"

#include <catch2/catch_test_macros.hpp>

#include <string>

namespace {

DirectorDesk::Storyboard::StoryboardSourceSnapshot CafeSnapshot(bool collapsedStreet = false) {
    const char* text =
        "# 我的短片\n\n"
        "## [scene:scene-cafe-day] 咖啡馆\n\n"
        "### [shot:shot-cafe-001] 过肩\n\n"
        "### [shot:shot-cafe-002] 特写\n\n"
        "## [scene:scene-street-night] 街道\n\n"
        "### [shot:shot-street-001] 俯拍\n\n";
    const auto parsed = DirectorDesk::Script::Parser::Parse(text);
    REQUIRE(parsed.completed);
    DirectorDesk::Storyboard::StoryboardSourceSnapshot snapshot;
    snapshot.documentTitle = parsed.snapshot.documentTitle;
    snapshot.scriptValid = true;
    snapshot.selectedShotId = "shot-cafe-001";
    int sceneIndex = 1;
    for (const auto& scene : parsed.snapshot.scenes) {
        DirectorDesk::Storyboard::SceneSource item;
        item.id = scene.id;
        item.title = scene.title;
        item.index = sceneIndex++;
        item.collapsed = collapsedStreet && scene.id == "scene-street-night";
        int shotIndex = 1;
        for (const auto& shot : scene.shots) {
            DirectorDesk::Storyboard::ShotSource shotItem;
            shotItem.id = shot.id;
            shotItem.title = shot.title;
            shotItem.indexInScene = shotIndex++;
            if (shot.id == "shot-cafe-001") {
                shotItem.cameraId = "cam-1";
                shotItem.cameraExists = true;
            }
            item.shots.push_back(shotItem);
        }
        snapshot.scenes.push_back(std::move(item));
    }
    return snapshot;
}

DirectorDesk::Storyboard::StoryboardSourceSnapshot ManyShots(int count) {
    DirectorDesk::Storyboard::StoryboardSourceSnapshot snapshot;
    snapshot.documentTitle = "压力";
    DirectorDesk::Storyboard::SceneSource scene;
    scene.id = "scene-many";
    scene.title = "大场";
    for (int i = 0; i < count; ++i) {
        DirectorDesk::Storyboard::ShotSource shot;
        shot.id = "shot-" + std::to_string(i);
        shot.title = "镜头" + std::to_string(i);
        shot.indexInScene = i + 1;
        shot.cameraId = "cam-1";
        shot.cameraExists = true;
        scene.shots.push_back(shot);
    }
    snapshot.scenes.push_back(std::move(scene));
    return snapshot;
}

} // namespace

TEST_CASE("Chinese script builds an ordered left-to-right tree", "[storyboard][layout]") {
    const auto layout = DirectorDesk::Storyboard::BuildLayout(CafeSnapshot());
    REQUIRE_FALSE(DirectorDesk::Storyboard::CardsOverlap(layout));
    const auto* first = DirectorDesk::Storyboard::FindCard(layout, "shot-cafe-001");
    const auto* second = DirectorDesk::Storyboard::FindCard(layout, "shot-cafe-002");
    const auto* third = DirectorDesk::Storyboard::FindCard(layout, "shot-street-001");
    REQUIRE(first != nullptr);
    REQUIRE(second != nullptr);
    REQUIRE(third != nullptr);
    REQUIRE(first->y < second->y);
    REQUIRE(second->y < third->y);
    REQUIRE(first->x > DirectorDesk::Storyboard::FindCard(layout, "scene-cafe-day")->x);
}

TEST_CASE("Script edits resync without overlapping cards", "[storyboard][layout]") {
    auto snapshot = CafeSnapshot();
    DirectorDesk::Storyboard::Document document;
    document.ApplySource(snapshot);
    REQUIRE_FALSE(DirectorDesk::Storyboard::CardsOverlap(document.Layout()));
    snapshot.scenes.front().title = "改名咖啡馆";
    snapshot.scenes.front().shots.erase(snapshot.scenes.front().shots.begin());
    document.ApplySource(snapshot);
    REQUIRE(DirectorDesk::Storyboard::FindCard(document.Layout(), "shot-cafe-001") == nullptr);
    REQUIRE(DirectorDesk::Storyboard::FindCard(document.Layout(), "shot-cafe-002") != nullptr);
    REQUIRE_FALSE(DirectorDesk::Storyboard::CardsOverlap(document.Layout()));
}

TEST_CASE("Collapsed scenes hide shots but export layout keeps them", "[storyboard][layout]") {
    auto snapshot = CafeSnapshot(true);
    DirectorDesk::Storyboard::Document document;
    document.ApplySource(snapshot);
    REQUIRE(DirectorDesk::Storyboard::FindCard(document.Layout(), "shot-street-001") == nullptr);
    const auto exported = document.ExportLayout();
    REQUIRE(DirectorDesk::Storyboard::FindCard(exported, "shot-street-001") != nullptr);
}

TEST_CASE("Invalid script keeps the last valid canvas", "[storyboard]") {
    DirectorDesk::Storyboard::Document document;
    document.ApplySource(CafeSnapshot());
    DirectorDesk::Storyboard::StoryboardSourceSnapshot bad;
    bad.scriptValid = false;
    document.ApplySource(bad);
    REQUIRE(document.HeldLastValid());
    REQUIRE(DirectorDesk::Storyboard::FindCard(document.Layout(), "shot-cafe-001") != nullptr);
}

TEST_CASE("Director changes stale only linked shots", "[storyboard]") {
    DirectorDesk::Storyboard::Document document;
    document.ApplySource(CafeSnapshot());
    document.MarkLinkedStale();
    REQUIRE(document.Thumbnail("shot-cafe-001") != nullptr);
    REQUIRE(document.Thumbnail("shot-cafe-001")->status ==
            DirectorDesk::Storyboard::PreviewStatus::Stale);
    REQUIRE(document.Thumbnail("shot-street-001") == nullptr);
}

TEST_CASE("Scheduler debounces and allows one task per frame", "[storyboard]") {
    DirectorDesk::Storyboard::ThumbnailScheduler scheduler;
    scheduler.NotifyBusy(1000);
    REQUIRE_FALSE(scheduler.ShouldRun(1200));
    REQUIRE(scheduler.ShouldRun(1400));
    scheduler.ConsumeFrame();
    REQUIRE_FALSE(scheduler.ShouldRun(1500));
    scheduler.BeginFrame();
    REQUIRE(scheduler.ShouldRun(1500));
}

TEST_CASE("200 shots layout and visible culling stay deterministic", "[storyboard][layout]") {
    const auto layout = DirectorDesk::Storyboard::BuildLayout(ManyShots(200));
    REQUIRE(layout.cards.size() >= 202);
    REQUIRE_FALSE(DirectorDesk::Storyboard::CardsOverlap(layout));
    DirectorDesk::Storyboard::ViewRect view;
    view.x = 0;
    view.y = 0;
    view.w = 900;
    view.h = 400;
    const auto visible = DirectorDesk::Storyboard::VisibleCardIds(layout, view);
    REQUIRE(visible.size() < layout.cards.size());
    REQUIRE_FALSE(visible.empty());
}

TEST_CASE("Board compose fills an opaque canvas without UI chrome", "[storyboard][export]") {
    const auto layout = DirectorDesk::Storyboard::BuildLayout(CafeSnapshot());
    DirectorDesk::Storyboard::BoardComposeRequest request;
    request.layout = layout;
    auto composed = DirectorDesk::Storyboard::ComposeBoard(request);
    REQUIRE(composed.IsOk());
    REQUIRE(composed.Value().pixels.width > 10);
    REQUIRE(composed.Value().pixels.height > 10);
    REQUIRE(composed.Value().pixels.rgba.size() ==
            composed.Value().pixels.width * composed.Value().pixels.height * 4u);
    REQUIRE(composed.Value().pixels.rgba[0] == 28);
    REQUIRE(composed.Value().pixels.rgba[3] == 255);
}

TEST_CASE("Collapsed export still counts every shot preview issue", "[storyboard]") {
    DirectorDesk::Storyboard::Document document;
    document.ApplySource(CafeSnapshot(true));
    REQUIRE(DirectorDesk::Storyboard::FindCard(document.Layout(), "shot-street-001") == nullptr);
    REQUIRE(document.CountExportPreviewIssues().Total() == 3);
}

TEST_CASE("Export preview issues count missing and stale shots", "[storyboard]") {
    DirectorDesk::Storyboard::Document document;
    document.ApplySource(CafeSnapshot());
    const auto issues = document.CountExportPreviewIssues();
    REQUIRE(issues.missing == 2);
    REQUIRE(issues.stale == 1);
    REQUIRE(issues.Total() == 3);
}

TEST_CASE("Fit and focus keep a selected shot in view", "[storyboard][layout]") {
    const auto layout = DirectorDesk::Storyboard::BuildLayout(CafeSnapshot());
    float panX = 0;
    float panY = 0;
    float zoom = 1;
    DirectorDesk::Storyboard::FitView(layout, 800, 600, panX, panY, zoom);
    REQUIRE(zoom <= 2.0f);
    DirectorDesk::Storyboard::FocusCard(layout, "shot-cafe-002", 800, 600, panX, panY, 1.0f);
    const auto* card = DirectorDesk::Storyboard::FindCard(layout, "shot-cafe-002");
    const float sx = panX + card->x;
    REQUIRE(sx > -card->w);
    REQUIRE(sx < 800);
}
