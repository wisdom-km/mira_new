// ShotExportTests: Implementation for the DirectorDesk Export module.
// This file owns project behavior only; keep platform and dependency boundaries explicit.
// Contract coverage: export resolutions and output names map to stable file contracts.


#include "DirectorDesk/Export/ShotExport.h"
#include "DirectorDesk/Platform/Paths.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <nlohmann/json.hpp>

TEST_CASE("Export resolutions match 1080p and 2K", "[export]") {
    const auto hd = DirectorDesk::Export::SizeFor(DirectorDesk::Export::ShotResolution::Hd1080);
    const auto uhd = DirectorDesk::Export::SizeFor(DirectorDesk::Export::ShotResolution::Uhd2k);
    REQUIRE(hd.width == 1920);
    REQUIRE(hd.height == 1080);
    REQUIRE(uhd.width == 2560);
    REQUIRE(uhd.height == 1440);
    const auto target = DirectorDesk::Export::MakeOffscreenTarget(
        DirectorDesk::Export::ShotResolution::Hd1080, true);
    REQUIRE(target.width == 1920);
    REQUIRE(target.transparentBackground);
    REQUIRE(target.kind == DirectorDesk::Renderer::RenderTargetKind::Offscreen);
}

TEST_CASE("Export file names include Chinese project and resolution", "[export]") {
    const std::string name = DirectorDesk::Export::DefaultShotFileName(
        "咖啡馆", "shot-cafe-001", DirectorDesk::Export::ShotResolution::Uhd2k, true);
    REQUIRE(name.find("咖啡馆") != std::string::npos);
    REQUIRE(name.find("shot-cafe-001") != std::string::npos);
    REQUIRE(name.find("2560x1440") != std::string::npos);
    REQUIRE(name.find("alpha") != std::string::npos);
}

TEST_CASE("35mm focal length matches 24mm-height formula", "[export][package]") {
    const float focal = DirectorDesk::Export::VerticalFovToFocalLength35mm(45.0f);
    const float hfov = DirectorDesk::Export::HorizontalFovDegrees(45.0f, 16.0f / 9.0f);
    REQUIRE(focal == Catch::Approx(28.968f).margin(0.05f));
    REQUIRE(hfov == Catch::Approx(72.734f).margin(0.2f));
}

TEST_CASE("WriteShotPackage writes PNG and JSON", "[export][package]") {
    auto temp = DirectorDesk::Platform::Paths::TemporaryDirectory();
    REQUIRE(temp.IsOk());
    const std::string dir =
        DirectorDesk::Platform::Paths::Join(temp.Value(), "导演台镜头包_DirectorDesk");
    REQUIRE(DirectorDesk::Platform::Paths::CreateDirectories(dir).IsOk());

    DirectorDesk::Renderer::PixelBuffer pixels;
    pixels.width = 2;
    pixels.height = 2;
    pixels.rgba = {255, 0, 0, 255, 0, 255, 0, 0, 0, 0, 255, 255, 255, 255, 255, 0};

    DirectorDesk::Export::ShotPackageInput input;
    input.generatedBy = "DirectorDesk test";
    input.generatedAt = "2026-10-01T12:00:00Z";
    input.projectName = "咖啡馆短片";
    input.projectId = "proj-test";
    input.sceneId = "scene-cafe-day";
    input.sceneTitle = "咖啡馆 - 日 - 内";
    input.shotId = "shot-cafe-001";
    input.shotTitle = "过肩";
    input.shotBody = "A 坐在窗边。";
    input.meta.push_back({"景别", "中景"});
    input.meta.push_back({"提示词", "暖色午后光"});
    input.prompt = "暖色午后光";
    input.imageWidth = 2;
    input.imageHeight = 2;
    input.verticalFovDegrees = 45.0f;
    input.aspect = 16.0f / 9.0f;
    input.cameraId = "camera-main";
    input.cameraName = "主镜头";
    DirectorDesk::Export::ShotPackageNode hidden;
    hidden.id = "node-hidden";
    hidden.visible = false;
    input.sceneNodes.push_back(hidden);
    DirectorDesk::Export::ShotPackageNode visible;
    visible.id = "node-chair-01";
    visible.name = "椅子";
    visible.assetId = "basic-chair";
    visible.assetName = "基础椅子";
    visible.visible = true;
    input.sceneNodes.push_back(visible);

    auto written = DirectorDesk::Export::WriteShotPackage(dir, input, pixels);
    REQUIRE(written.IsOk());
    REQUIRE(DirectorDesk::Platform::Paths::Exists(written.Value().pngPath));
    REQUIRE(DirectorDesk::Platform::Paths::Exists(written.Value().jsonPath));

    auto jsonText = DirectorDesk::Platform::Paths::ReadTextFile(written.Value().jsonPath);
    REQUIRE(jsonText.IsOk());
    const nlohmann::json root = nlohmann::json::parse(jsonText.Value());
    REQUIRE(root["format"] == "DirectorDeskShotPackage");
    REQUIRE(root["formatVersion"] == 1);
    REQUIRE(root["shot"]["prompt"] == "暖色午后光");
    REQUIRE(root["camera"]["focalLength35mmEquivalent"].get<float>() ==
            Catch::Approx(DirectorDesk::Export::VerticalFovToFocalLength35mm(45.0f)).margin(0.01f));
    REQUIRE(root["sceneNodes"].size() == 1);
    REQUIRE(root["sceneNodes"][0]["id"] == "node-chair-01");
}

TEST_CASE("WriteBoardIndex lists shots", "[export][package]") {
    auto temp = DirectorDesk::Platform::Paths::TemporaryDirectory();
    REQUIRE(temp.IsOk());
    const std::string dir =
        DirectorDesk::Platform::Paths::Join(temp.Value(), "导演台总览_DirectorDesk");
    REQUIRE(DirectorDesk::Platform::Paths::CreateDirectories(dir).IsOk());
    const std::string path = DirectorDesk::Platform::Paths::Join(dir, "board.json");
    std::vector<DirectorDesk::Export::BoardIndexShot> shots;
    shots.push_back({"shot-a", "过肩", "", "中景 · 推 · 3s"});
    REQUIRE(DirectorDesk::Export::WriteBoardIndex(path, "board.png", shots).IsOk());
    auto jsonText = DirectorDesk::Platform::Paths::ReadTextFile(path);
    REQUIRE(jsonText.IsOk());
    const nlohmann::json root = nlohmann::json::parse(jsonText.Value());
    REQUIRE(root["format"] == "DirectorDeskBoardIndex");
    REQUIRE(root["shots"][0]["id"] == "shot-a");
    REQUIRE(root["shots"][0]["metaLine"] == "中景 · 推 · 3s");
    REQUIRE(root["shots"][0]["image"] == "board.png");
}

TEST_CASE("WriteBoardPdf writes A4 landscape MediaBox", "[export][pdf]") {
    auto temp = DirectorDesk::Platform::Paths::TemporaryDirectory();
    REQUIRE(temp.IsOk());
    const std::string dir =
        DirectorDesk::Platform::Paths::Join(temp.Value(), "导演台PDF_DirectorDesk");
    REQUIRE(DirectorDesk::Platform::Paths::CreateDirectories(dir).IsOk());
    const std::string path = DirectorDesk::Platform::Paths::Join(dir, "board.pdf");

    DirectorDesk::Renderer::PixelBuffer page;
    page.width = 2;
    page.height = 2;
    page.rgba = {255, 0, 0, 255, 0, 255, 0, 255, 0, 0, 255, 255, 255, 255, 255, 255};
    REQUIRE(DirectorDesk::Export::WriteBoardPdf(path, {page}).IsOk());

    auto bytes = DirectorDesk::Platform::Paths::ReadBinaryFile(path);
    REQUIRE(bytes.IsOk());
    const std::string text(bytes.Value().begin(), bytes.Value().end());
    REQUIRE(text.find("%PDF-1.4") == 0);
    REQUIRE(text.find("/MediaBox [0 0 842 595]") != std::string::npos);
}
