// ServiceTests: Implementation for the DirectorDesk AI module.
// This file owns project behavior only; keep platform and dependency boundaries explicit.
// Contract coverage: provider-neutral image/video services preserve request data and lifecycle state.


#include "DirectorDesk/AI/MockServices.h"
#include "DirectorDesk/AI/NullServices.h"

#include <catch2/catch_test_macros.hpp>

#include <string>

using namespace DirectorDesk;

namespace {

AI::ImageGenRequest SampleImage() {
    AI::ImageGenRequest request;
    request.prompt = "cafe wide shot";
    request.shot.shotId = "shot-1";
    request.shot.sceneId = "scene-1";
    request.shot.shotTitle = "开场";
    request.shot.shotText = "咖啡馆外景";
    request.shot.cameraId = "cam-a";
    request.shot.cameraPresetId = "wide";
    request.shot.projectName = "cafe";
    AI::ReferenceImage image;
    image.path = "D:/导出/镜头一.png";
    image.width = 1920;
    image.height = 1080;
    request.references.push_back(std::move(image));
    return request;
}

AI::VideoGenRequest SampleVideo() {
    AI::VideoGenRequest request;
    request.prompt = "dolly in";
    request.shot = SampleImage().shot;
    request.references = SampleImage().references;
    request.durationMs = 3000;
    request.frameRate = 24;
    return request;
}

} // namespace

TEST_CASE("Null image service rejects work without a vendor", "[ai]") {
    auto service = AI::CreateNullImageGenService();
    const auto submitted = service->Submit(SampleImage());
    REQUIRE_FALSE(submitted.IsOk());
    REQUIRE(submitted.GetError().code == Core::ErrorCode::Unsupported);
    REQUIRE(submitted.GetError().userMessage == "当前版本未接入生成服务");
    REQUIRE(submitted.GetError().technicalMessage.find("openai") == std::string::npos);
}

TEST_CASE("Null video service rejects work without a vendor", "[ai]") {
    auto service = AI::CreateNullVideoGenService();
    REQUIRE_FALSE(service->Submit(SampleVideo()).IsOk());
    REQUIRE_FALSE(service->Cancel("vid-1").IsOk());
}

TEST_CASE("Mock image service completes asynchronously and keeps shot metadata", "[ai]") {
    AI::MockImageGenService service;
    const auto id = service.Submit(SampleImage());
    REQUIRE(id.IsOk());

    auto progress = service.Progress(id.Value());
    REQUIRE(progress.IsOk());
    REQUIRE(progress.Value().status == AI::GenJobStatus::Queued);
    REQUIRE_FALSE(service.TakeResult(id.Value()).IsOk());

    service.Pump();
    progress = service.Progress(id.Value());
    REQUIRE(progress.Value().status == AI::GenJobStatus::Running);

    service.Pump();
    progress = service.Progress(id.Value());
    REQUIRE(progress.Value().status == AI::GenJobStatus::Succeeded);
    REQUIRE(progress.Value().ratio == 1.0f);

    const auto result = service.TakeResult(id.Value());
    REQUIRE(result.IsOk());
    REQUIRE(result.Value().shot.shotId == "shot-1");
    REQUIRE(result.Value().shot.shotTitle == "开场");
    REQUIRE(result.Value().references.size() == 1);
    REQUIRE(result.Value().references[0].path == "D:/导出/镜头一.png");
    REQUIRE(result.Value().width == 1920);
    REQUIRE(result.Value().outputPath.find("://") == std::string::npos);
    REQUIRE_FALSE(service.Progress(id.Value()).IsOk());
}

TEST_CASE("Mock image service reports scripted failures", "[ai]") {
    AI::MockImageGenService service;
    service.FailNext(Core::Error::Make(Core::ErrorCode::Internal, "mock fail", "生成失败"));
    const auto id = service.Submit(SampleImage());
    REQUIRE(id.IsOk());
    service.Pump();
    service.Pump();
    const auto result = service.TakeResult(id.Value());
    REQUIRE_FALSE(result.IsOk());
    REQUIRE(result.GetError().userMessage == "生成失败");
}

TEST_CASE("Mock image service can cancel before completion", "[ai]") {
    AI::MockImageGenService service;
    const auto id = service.Submit(SampleImage());
    REQUIRE(service.Cancel(id.Value()).IsOk());
    REQUIRE(service.Progress(id.Value()).Value().status == AI::GenJobStatus::Cancelled);
    const auto result = service.TakeResult(id.Value());
    REQUIRE_FALSE(result.IsOk());
    REQUIRE(result.GetError().code == Core::ErrorCode::Cancelled);
}

TEST_CASE("Mock image service rejects remote reference paths", "[ai]") {
    AI::MockImageGenService service;
    AI::ImageGenRequest request = SampleImage();
    request.references[0].path = "https://example.com/ref.png";
    const auto submitted = service.Submit(request);
    REQUIRE_FALSE(submitted.IsOk());
    REQUIRE(submitted.GetError().code == Core::ErrorCode::InvalidArgument);
}

TEST_CASE("Mock video service completes and preserves duration metadata", "[ai]") {
    AI::MockVideoGenService service;
    const auto id = service.Submit(SampleVideo());
    REQUIRE(id.IsOk());
    service.Pump();
    service.Pump();
    const auto result = service.TakeResult(id.Value());
    REQUIRE(result.IsOk());
    REQUIRE(result.Value().durationMs == 3000);
    REQUIRE(result.Value().frameRate == 24);
    REQUIRE(result.Value().shot.cameraPresetId == "wide");
    REQUIRE(result.Value().outputPath.find(".mp4") != std::string::npos);
}

TEST_CASE("Unsafe reference path helper rejects URLs", "[ai]") {
    REQUIRE(AI::IsSafeReferencePath("C:/shots/a.png"));
    REQUIRE(AI::IsSafeReferencePath(""));
    REQUIRE_FALSE(AI::IsSafeReferencePath("https://cdn.example/a.png"));
    REQUIRE(std::string(AI::JobStatusId(AI::GenJobStatus::Running)) == "running");
    REQUIRE(AI::IsTerminal(AI::GenJobStatus::Failed));
    REQUIRE_FALSE(AI::IsTerminal(AI::GenJobStatus::Queued));
}
