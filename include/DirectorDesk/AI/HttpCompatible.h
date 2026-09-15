// HttpCompatible: OpenAI-compatible HTTPS image/video generation. No vendor SDK types.

#pragma once

#include "DirectorDesk/AI/Types.h"
#include "DirectorDesk/Core/Result.h"
#include "DirectorDesk/Platform/IHttpClient.h"

#include <atomic>
#include <string>

namespace DirectorDesk::AI {

struct HttpAiConfig {
    std::string baseUrl;
    std::string apiKey;
    std::string imageModel = "gpt-image-1";
    std::string videoModel = "sora-2";
    std::string chatModel = "gpt-4o-mini";
    std::string outputDirectory;
};

struct StoryboardChatRequest {
    std::string scriptText;
    std::string projectName;
    std::string systemPrompt;
    std::string outputPath;
};

[[nodiscard]] bool IsMockProvider(const std::string& provider);
[[nodiscard]] bool IsOpenAiCompatProvider(const std::string& provider);
[[nodiscard]] std::string NormalizeAiProvider(const std::string& provider);

Core::Result<ImageGenResult> ExecuteImageGeneration(Platform::IHttpClient& http,
                                                    const HttpAiConfig& config,
                                                    const ImageGenRequest& request,
                                                    const std::atomic<bool>* cancel);

Core::Result<VideoGenResult> ExecuteVideoGeneration(Platform::IHttpClient& http,
                                                    const HttpAiConfig& config,
                                                    const VideoGenRequest& request,
                                                    const std::atomic<bool>* cancel);

Core::Result<ImageGenResult> ExecuteMockImage(const ImageGenRequest& request,
                                              const std::string& outputDirectory);

Core::Result<VideoGenResult> ExecuteMockVideo(const VideoGenRequest& request,
                                              const std::string& outputDirectory);

Core::Result<std::string> ExecuteStoryboardChat(Platform::IHttpClient& http,
                                                const HttpAiConfig& config,
                                                const StoryboardChatRequest& request,
                                                const std::atomic<bool>* cancel);

Core::Result<std::string> ExecuteMockStoryboardJson(const StoryboardChatRequest& request);

} // namespace DirectorDesk::AI
