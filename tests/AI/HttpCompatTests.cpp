// HttpCompatibleTests: OpenAI-compat adapter against a fake HTTPS client.

#include "Asset/MockHttpClient.h"
#include "DirectorDesk/AI/HttpCompatible.h"
#include "DirectorDesk/Platform/Paths.h"

#include <catch2/catch_test_macros.hpp>

#include <string>

using namespace DirectorDesk;

namespace {

std::string OutputDir() {
    auto temp = Platform::Paths::TemporaryDirectory();
    REQUIRE(temp.IsOk());
    const std::string dir =
        Platform::Paths::Join(temp.Value(), "导演台AI DirectorDesk/http-out");
    REQUIRE(Platform::Paths::CreateDirectories(dir).IsOk());
    return dir;
}

AI::ImageGenRequest SampleImage() {
    AI::ImageGenRequest request;
    request.prompt = "暖色午后";
    request.shot.shotId = "shot-cafe-001";
    request.shot.shotTitle = "过肩";
    request.width = 1920;
    request.height = 1080;
    return request;
}

} // namespace

TEST_CASE("Http image generation writes b64 png", "[ai][http]") {
    Tests::MockHttpClient http;
    const std::string pngB64 =
        "iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAYAAAAfFcSJAAAADUlEQVR42mP8z8BQDwAEhQGAhKmMIQAAAABJRU5ErkJggg==";
    const std::string body = std::string("{\"data\":[{\"b64_json\":\"") + pngB64 + "\"}]}";
    Tests::MockHttpClient::Route route;
    route.status = 200;
    route.body.assign(body.begin(), body.end());
    http.routes["https://api.example.com/v1/images/generations"] = route;

    AI::HttpAiConfig config;
    config.baseUrl = "https://api.example.com";
    config.apiKey = "sk-test";
    config.outputDirectory = OutputDir();
    auto result = AI::ExecuteImageGeneration(http, config, SampleImage(), nullptr);
    REQUIRE(result.IsOk());
    REQUIRE(Platform::Paths::Exists(result.Value().outputPath));
    REQUIRE(result.Value().outputPath.find("://") == std::string::npos);
    REQUIRE(http.lastPostBody.find("暖色午后") != std::string::npos);
    REQUIRE(http.lastPostBody.find("sk-test") == std::string::npos);
}

TEST_CASE("Http image generation rejects missing key and remote refs", "[ai][http]") {
    Tests::MockHttpClient http;
    AI::HttpAiConfig config;
    config.baseUrl = "https://api.example.com";
    config.outputDirectory = OutputDir();
    auto missing = AI::ExecuteImageGeneration(http, config, SampleImage(), nullptr);
    REQUIRE_FALSE(missing.IsOk());
    REQUIRE(missing.GetError().userMessage == "请先填写 API 密钥");

    config.apiKey = "sk-test";
    config.baseUrl = "http://api.example.com";
    REQUIRE_FALSE(AI::ExecuteImageGeneration(http, config, SampleImage(), nullptr).IsOk());

    config.baseUrl = "https://api.example.com";
    AI::ImageGenRequest request = SampleImage();
    AI::ReferenceImage image;
    image.path = "https://cdn.example/a.png";
    request.references.push_back(image);
    REQUIRE_FALSE(AI::ExecuteImageGeneration(http, config, request, nullptr).IsOk());
}

TEST_CASE("Mock image generation writes a local png", "[ai][http]") {
    auto result = AI::ExecuteMockImage(SampleImage(), OutputDir());
    REQUIRE(result.IsOk());
    REQUIRE(Platform::Paths::Exists(result.Value().outputPath));
    REQUIRE(Platform::Paths::ExtensionLower(result.Value().outputPath) == ".png");
}

TEST_CASE("Http storyboard chat writes DirectorDesk import json", "[ai][http][skill]") {
    Tests::MockHttpClient http;
    const std::string payload =
        "{\"choices\":[{\"message\":{\"content\":\"{\\\"format\\\":\\\"DirectorDeskStoryboardImport\\\","
        "\\\"formatVersion\\\":1,\\\"title\\\":\\\"AI片\\\",\\\"scenes\\\":[{\\\"id\\\":\\\"scene-a\\\","
        "\\\"title\\\":\\\"场\\\",\\\"shots\\\":[{\\\"id\\\":\\\"shot-a\\\",\\\"title\\\":\\\"镜\\\"}]}]}\"}}]}";
    Tests::MockHttpClient::Route route;
    route.status = 200;
    route.body.assign(payload.begin(), payload.end());
    http.routes["https://api.example.com/v1/chat/completions"] = route;

    AI::HttpAiConfig config;
    config.baseUrl = "https://api.example.com";
    config.apiKey = "sk-test";
    config.chatModel = "deepseek-chat";
    AI::StoryboardChatRequest request;
    request.scriptText = "咖啡馆。";
    request.projectName = "测试";
    request.outputPath = Platform::Paths::Join(OutputDir(), "storyboard-import.json");
    auto result = AI::ExecuteStoryboardChat(http, config, request, nullptr);
    REQUIRE(result.IsOk());
    REQUIRE(Platform::Paths::Exists(result.Value()));
    REQUIRE(http.lastPostUrl.find("/v1/chat/completions") != std::string::npos);
    REQUIRE(http.lastPostBody.find("deepseek-chat") != std::string::npos);
    REQUIRE(http.lastPostBody.find("sk-test") == std::string::npos);
    auto text = Platform::Paths::ReadTextFile(result.Value());
    REQUIRE(text.IsOk());
    REQUIRE(text.Value().find("DirectorDeskStoryboardImport") != std::string::npos);
}

TEST_CASE("Http storyboard chat rejects missing key", "[ai][http][skill]") {
    Tests::MockHttpClient http;
    AI::HttpAiConfig config;
    config.baseUrl = "https://api.example.com";
    AI::StoryboardChatRequest request;
    request.outputPath = Platform::Paths::Join(OutputDir(), "storyboard-import.json");
    auto missing = AI::ExecuteStoryboardChat(http, config, request, nullptr);
    REQUIRE_FALSE(missing.IsOk());
    REQUIRE(missing.GetError().userMessage == "请先填写 API 密钥");
}
