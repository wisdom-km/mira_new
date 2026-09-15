// HttpCompatible: OpenAI-compatible HTTPS generation.

#include "DirectorDesk/AI/HttpCompatible.h"

#include "DirectorDesk/Core/Error.h"
#include "DirectorDesk/Platform/Paths.h"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <chrono>
#include <cctype>
#include <thread>

namespace DirectorDesk::AI {
namespace {

const std::uint8_t kPng1x1[] = {
    0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a, 0x00, 0x00, 0x00, 0x0d, 0x49, 0x48, 0x44,
    0x52, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x08, 0x06, 0x00, 0x00, 0x00, 0x1f,
    0x15, 0xc4, 0x89, 0x00, 0x00, 0x00, 0x0a, 0x49, 0x44, 0x41, 0x54, 0x78, 0x9c, 0x63, 0x00,
    0x01, 0x00, 0x00, 0x05, 0x00, 0x01, 0x0d, 0x0a, 0x2d, 0xb4, 0x00, 0x00, 0x00, 0x00, 0x49,
    0x45, 0x4e, 0x44, 0xae, 0x42, 0x60, 0x82};

const char kBase64Table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

Core::Error Fail(Core::ErrorCode code, const std::string& technical, const std::string& user) {
    return Core::Error::Make(code, technical, user);
}

bool ReferencesSafe(const std::vector<ReferenceImage>& references) {
    for (const ReferenceImage& image : references) {
        if (!IsSafeReferencePath(image.path)) {
            return false;
        }
    }
    return true;
}

std::string TrimSlash(std::string url) {
    while (!url.empty() && url.back() == '/') {
        url.pop_back();
    }
    return url;
}

std::string JoinUrl(const std::string& base, const std::string& path) {
    const std::string root = TrimSlash(base);
    if (path.empty()) {
        return root;
    }
    if (path[0] == '/') {
        return root + path;
    }
    return root + "/" + path;
}

std::string ImageSizeToken(std::uint32_t width, std::uint32_t height) {
    if (width <= 1024 && height <= 1024) {
        return "1024x1024";
    }
    if (width >= height) {
        return "1536x1024";
    }
    return "1024x1536";
}

std::string Base64Encode(const std::vector<std::uint8_t>& data) {
    std::string out;
    out.reserve(((data.size() + 2) / 3) * 4);
    std::size_t i = 0;
    while (i + 2 < data.size()) {
        const unsigned n = (static_cast<unsigned>(data[i]) << 16) |
                           (static_cast<unsigned>(data[i + 1]) << 8) | data[i + 2];
        out.push_back(kBase64Table[(n >> 18) & 63]);
        out.push_back(kBase64Table[(n >> 12) & 63]);
        out.push_back(kBase64Table[(n >> 6) & 63]);
        out.push_back(kBase64Table[n & 63]);
        i += 3;
    }
    if (i < data.size()) {
        unsigned n = static_cast<unsigned>(data[i]) << 16;
        if (i + 1 < data.size()) {
            n |= static_cast<unsigned>(data[i + 1]) << 8;
        }
        out.push_back(kBase64Table[(n >> 18) & 63]);
        out.push_back(kBase64Table[(n >> 12) & 63]);
        if (i + 1 < data.size()) {
            out.push_back(kBase64Table[(n >> 6) & 63]);
            out.push_back('=');
        } else {
            out += "==";
        }
    }
    return out;
}

int Base64Value(char ch) {
    if (ch >= 'A' && ch <= 'Z') {
        return ch - 'A';
    }
    if (ch >= 'a' && ch <= 'z') {
        return ch - 'a' + 26;
    }
    if (ch >= '0' && ch <= '9') {
        return ch - '0' + 52;
    }
    if (ch == '+') {
        return 62;
    }
    if (ch == '/') {
        return 63;
    }
    return -1;
}

Core::Result<std::vector<std::uint8_t>> Base64Decode(const std::string& text) {
    std::string packed;
    packed.reserve(text.size());
    for (char ch : text) {
        if (std::isspace(static_cast<unsigned char>(ch))) {
            continue;
        }
        packed.push_back(ch);
    }
    if (packed.size() % 4 != 0) {
        return Core::Result<std::vector<std::uint8_t>>::Fail(
            Fail(Core::ErrorCode::ParseFailure, "bad b64 length", "生成结果无法解码"));
    }
    std::vector<std::uint8_t> out;
    out.reserve(packed.size() / 4 * 3);
    for (std::size_t i = 0; i < packed.size(); i += 4) {
        const int a = Base64Value(packed[i]);
        const int b = Base64Value(packed[i + 1]);
        const int c = packed[i + 2] == '=' ? 0 : Base64Value(packed[i + 2]);
        const int d = packed[i + 3] == '=' ? 0 : Base64Value(packed[i + 3]);
        if (a < 0 || b < 0 || (packed[i + 2] != '=' && c < 0) || (packed[i + 3] != '=' && d < 0)) {
            return Core::Result<std::vector<std::uint8_t>>::Fail(
                Fail(Core::ErrorCode::ParseFailure, "bad b64 char", "生成结果无法解码"));
        }
        const unsigned n = (static_cast<unsigned>(a) << 18) | (static_cast<unsigned>(b) << 12) |
                           (static_cast<unsigned>(c) << 6) | static_cast<unsigned>(d);
        out.push_back(static_cast<std::uint8_t>((n >> 16) & 255));
        if (packed[i + 2] != '=') {
            out.push_back(static_cast<std::uint8_t>((n >> 8) & 255));
        }
        if (packed[i + 3] != '=') {
            out.push_back(static_cast<std::uint8_t>(n & 255));
        }
    }
    return Core::Result<std::vector<std::uint8_t>>::Ok(std::move(out));
}

Platform::HttpHeader Bearer(const std::string& apiKey) {
    Platform::HttpHeader header;
    header.name = "Authorization";
    header.value = "Bearer " + apiKey;
    return header;
}

Core::Result<nlohmann::json> ParseObject(const Platform::HttpGetResponse& response) {
    if (response.status < 200 || response.status >= 300) {
        std::string message = "HTTP " + std::to_string(response.status);
        try {
            const auto json = nlohmann::json::parse(response.body.begin(), response.body.end());
            if (json.contains("error") && json["error"].is_object() &&
                json["error"].contains("message") && json["error"]["message"].is_string()) {
                message = json["error"]["message"].get<std::string>();
            }
        } catch (const nlohmann::json::exception&) {
        }
        return Core::Result<nlohmann::json>::Fail(
            Fail(Core::ErrorCode::IoFailure, "http " + std::to_string(response.status), message));
    }
    try {
        auto json = nlohmann::json::parse(response.body.begin(), response.body.end());
        if (!json.is_object()) {
            return Core::Result<nlohmann::json>::Fail(
                Fail(Core::ErrorCode::ParseFailure, "json root", "生成服务返回无法解析"));
        }
        return Core::Result<nlohmann::json>::Ok(std::move(json));
    } catch (const nlohmann::json::exception& ex) {
        return Core::Result<nlohmann::json>::Fail(
            Fail(Core::ErrorCode::ParseFailure, ex.what(), "生成服务返回无法解析"));
    }
}

std::string FirstDataUrl(const nlohmann::json& json) {
    if (json.contains("data") && json["data"].is_array() && !json["data"].empty()) {
        const auto& item = json["data"][0];
        if (item.contains("url") && item["url"].is_string()) {
            return item["url"].get<std::string>();
        }
        if (item.contains("b64_json") && item["b64_json"].is_string()) {
            return {};
        }
    }
    if (json.contains("output") && json["output"].is_object() && json["output"].contains("url") &&
        json["output"]["url"].is_string()) {
        return json["output"]["url"].get<std::string>();
    }
    return {};
}

std::string FirstB64(const nlohmann::json& json) {
    if (json.contains("data") && json["data"].is_array() && !json["data"].empty()) {
        const auto& item = json["data"][0];
        if (item.contains("b64_json") && item["b64_json"].is_string()) {
            return item["b64_json"].get<std::string>();
        }
    }
    return {};
}

Core::Result<void> DownloadHttps(Platform::IHttpClient& http, const std::string& url,
                                 const std::string& outputPath, const std::string& apiKey,
                                 const std::atomic<bool>* cancel) {
    if (url.rfind("https://", 0) != 0) {
        return Core::Result<void>::Fail(
            Fail(Core::ErrorCode::InvalidArgument, "non-https result url", "生成结果不是 HTTPS"));
    }
    Platform::HttpGetRequest get;
    get.url = url;
    get.outputPath = outputPath;
    get.timeoutMs = 120000;
    get.cancel = cancel;
    if (!apiKey.empty()) {
        get.headers.push_back(Bearer(apiKey));
    }
    auto response = http.Get(get);
    if (!response.IsOk()) {
        return Core::Result<void>::Fail(response.GetError());
    }
    if (response.Value().status < 200 || response.Value().status >= 300) {
        return Core::Result<void>::Fail(Fail(Core::ErrorCode::IoFailure,
                                             "download status", "无法下载生成结果"));
    }
    return Core::Result<void>::Ok();
}

Core::Result<void> EnsureOutputDir(const std::string& directory) {
    if (directory.empty()) {
        return Core::Result<void>::Fail(
            Fail(Core::ErrorCode::InvalidArgument, "empty output dir", "生成输出目录无效"));
    }
    return Platform::Paths::CreateDirectories(directory);
}

std::string JobFile(const std::string& directory, const std::string& shotId, const char* ext) {
    const std::string stem = shotId.empty() ? "shot" : shotId;
    return Platform::Paths::Join(directory, stem + ext);
}

} // namespace

bool IsMockProvider(const std::string& provider) {
    return provider == "mock";
}

bool IsOpenAiCompatProvider(const std::string& provider) {
    return provider.empty() || provider == "openai-compat";
}

std::string NormalizeAiProvider(const std::string& provider) {
    if (IsMockProvider(provider)) {
        return "mock";
    }
    return "openai-compat";
}

Core::Result<ImageGenResult> ExecuteMockImage(const ImageGenRequest& request,
                                              const std::string& outputDirectory) {
    if (request.shot.shotId.empty()) {
        return Core::Result<ImageGenResult>::Fail(
            Fail(Core::ErrorCode::InvalidArgument, "missing shotId", "生成请求缺少镜头 ID"));
    }
    if (!ReferencesSafe(request.references)) {
        return Core::Result<ImageGenResult>::Fail(
            Fail(Core::ErrorCode::InvalidArgument, "remote reference path", "参考图只能使用本地路径"));
    }
    auto created = EnsureOutputDir(outputDirectory);
    if (!created.IsOk()) {
        return Core::Result<ImageGenResult>::Fail(created.GetError());
    }
    const std::string path = JobFile(outputDirectory, request.shot.shotId, "-ai.png");
    if (!request.references.empty() && !request.references[0].path.empty() &&
        Platform::Paths::Exists(request.references[0].path)) {
        auto copied = Platform::Paths::CopyFileUtf8(request.references[0].path, path);
        if (!copied.IsOk()) {
            return Core::Result<ImageGenResult>::Fail(copied.GetError());
        }
    } else {
        auto written = Platform::Paths::WriteBinaryFile(path, kPng1x1, sizeof(kPng1x1));
        if (!written.IsOk()) {
            return Core::Result<ImageGenResult>::Fail(written.GetError());
        }
    }
    ImageGenResult result;
    result.jobId = "mock-" + request.shot.shotId;
    result.outputPath = path;
    result.width = request.width;
    result.height = request.height;
    result.shot = request.shot;
    result.references = request.references;
    return Core::Result<ImageGenResult>::Ok(std::move(result));
}

Core::Result<VideoGenResult> ExecuteMockVideo(const VideoGenRequest& request,
                                              const std::string& outputDirectory) {
    if (request.shot.shotId.empty()) {
        return Core::Result<VideoGenResult>::Fail(
            Fail(Core::ErrorCode::InvalidArgument, "missing shotId", "生成请求缺少镜头 ID"));
    }
    if (!ReferencesSafe(request.references)) {
        return Core::Result<VideoGenResult>::Fail(
            Fail(Core::ErrorCode::InvalidArgument, "remote reference path", "参考图只能使用本地路径"));
    }
    auto created = EnsureOutputDir(outputDirectory);
    if (!created.IsOk()) {
        return Core::Result<VideoGenResult>::Fail(created.GetError());
    }
    const std::string path = JobFile(outputDirectory, request.shot.shotId, "-ai.mp4.txt");
    auto written = Platform::Paths::WriteTextFile(
        path, "DirectorDesk mock video for " + request.shot.shotId + "\n");
    if (!written.IsOk()) {
        return Core::Result<VideoGenResult>::Fail(written.GetError());
    }
    VideoGenResult result;
    result.jobId = "mock-" + request.shot.shotId;
    result.outputPath = path;
    result.width = request.width;
    result.height = request.height;
    result.durationMs = request.durationMs;
    result.frameRate = request.frameRate;
    result.shot = request.shot;
    result.references = request.references;
    return Core::Result<VideoGenResult>::Ok(std::move(result));
}

Core::Result<ImageGenResult> ExecuteImageGeneration(Platform::IHttpClient& http,
                                                    const HttpAiConfig& config,
                                                    const ImageGenRequest& request,
                                                    const std::atomic<bool>* cancel) {
    if (request.shot.shotId.empty()) {
        return Core::Result<ImageGenResult>::Fail(
            Fail(Core::ErrorCode::InvalidArgument, "missing shotId", "生成请求缺少镜头 ID"));
    }
    if (request.width == 0 || request.height == 0) {
        return Core::Result<ImageGenResult>::Fail(
            Fail(Core::ErrorCode::InvalidArgument, "invalid image size", "生成尺寸无效"));
    }
    if (!ReferencesSafe(request.references)) {
        return Core::Result<ImageGenResult>::Fail(
            Fail(Core::ErrorCode::InvalidArgument, "remote reference path", "参考图只能使用本地路径"));
    }
    if (config.apiKey.empty()) {
        return Core::Result<ImageGenResult>::Fail(
            Fail(Core::ErrorCode::NotInitialized, "missing api key", "请先填写 API 密钥"));
    }
    if (config.baseUrl.rfind("https://", 0) != 0) {
        return Core::Result<ImageGenResult>::Fail(
            Fail(Core::ErrorCode::InvalidArgument, "base url must be https", "API 地址必须是 HTTPS"));
    }
    auto created = EnsureOutputDir(config.outputDirectory);
    if (!created.IsOk()) {
        return Core::Result<ImageGenResult>::Fail(created.GetError());
    }

    nlohmann::json body;
    body["model"] = config.imageModel.empty() ? "gpt-image-1" : config.imageModel;
    body["prompt"] = request.prompt;
    body["size"] = ImageSizeToken(request.width, request.height);
    body["n"] = 1;
    body["response_format"] = "b64_json";
    if (!request.references.empty() && !request.references[0].path.empty() &&
        Platform::Paths::Exists(request.references[0].path)) {
        auto bytes = Platform::Paths::ReadBinaryFile(request.references[0].path);
        if (bytes.IsOk()) {
            body["image"] = Base64Encode(bytes.Value());
        }
    }

    Platform::HttpPostRequest post;
    post.url = JoinUrl(config.baseUrl, "/v1/images/generations");
    post.body = body.dump();
    post.timeoutMs = 120000;
    post.cancel = cancel;
    post.headers.push_back(Bearer(config.apiKey));
    auto posted = http.Post(post);
    if (!posted.IsOk()) {
        return Core::Result<ImageGenResult>::Fail(posted.GetError());
    }
    auto json = ParseObject(posted.Value());
    if (!json.IsOk()) {
        return Core::Result<ImageGenResult>::Fail(json.GetError());
    }

    ImageGenResult result;
    result.jobId = "img-" + request.shot.shotId;
    result.width = request.width;
    result.height = request.height;
    result.shot = request.shot;
    result.references = request.references;
    result.outputPath = JobFile(config.outputDirectory, request.shot.shotId, "-ai.png");

    const std::string b64 = FirstB64(json.Value());
    if (!b64.empty()) {
        auto decoded = Base64Decode(b64);
        if (!decoded.IsOk()) {
            return Core::Result<ImageGenResult>::Fail(decoded.GetError());
        }
        auto written = Platform::Paths::WriteBinaryFile(result.outputPath, decoded.Value().data(),
                                                        decoded.Value().size());
        if (!written.IsOk()) {
            return Core::Result<ImageGenResult>::Fail(written.GetError());
        }
        return Core::Result<ImageGenResult>::Ok(std::move(result));
    }
    const std::string url = FirstDataUrl(json.Value());
    if (url.empty()) {
        return Core::Result<ImageGenResult>::Fail(
            Fail(Core::ErrorCode::ParseFailure, "no image payload", "生成服务没有返回图像"));
    }
    auto downloaded = DownloadHttps(http, url, result.outputPath, config.apiKey, cancel);
    if (!downloaded.IsOk()) {
        return Core::Result<ImageGenResult>::Fail(downloaded.GetError());
    }
    return Core::Result<ImageGenResult>::Ok(std::move(result));
}

Core::Result<VideoGenResult> ExecuteVideoGeneration(Platform::IHttpClient& http,
                                                    const HttpAiConfig& config,
                                                    const VideoGenRequest& request,
                                                    const std::atomic<bool>* cancel) {
    if (request.shot.shotId.empty()) {
        return Core::Result<VideoGenResult>::Fail(
            Fail(Core::ErrorCode::InvalidArgument, "missing shotId", "生成请求缺少镜头 ID"));
    }
    if (request.width == 0 || request.height == 0 || request.durationMs == 0) {
        return Core::Result<VideoGenResult>::Fail(
            Fail(Core::ErrorCode::InvalidArgument, "invalid video params", "视频生成参数无效"));
    }
    if (!ReferencesSafe(request.references)) {
        return Core::Result<VideoGenResult>::Fail(
            Fail(Core::ErrorCode::InvalidArgument, "remote reference path", "参考图只能使用本地路径"));
    }
    if (config.apiKey.empty()) {
        return Core::Result<VideoGenResult>::Fail(
            Fail(Core::ErrorCode::NotInitialized, "missing api key", "请先填写 API 密钥"));
    }
    if (config.baseUrl.rfind("https://", 0) != 0) {
        return Core::Result<VideoGenResult>::Fail(
            Fail(Core::ErrorCode::InvalidArgument, "base url must be https", "API 地址必须是 HTTPS"));
    }
    auto created = EnsureOutputDir(config.outputDirectory);
    if (!created.IsOk()) {
        return Core::Result<VideoGenResult>::Fail(created.GetError());
    }

    nlohmann::json body;
    body["model"] = config.videoModel.empty() ? "sora-2" : config.videoModel;
    body["prompt"] = request.prompt;
    body["seconds"] = std::max(1u, (request.durationMs + 500) / 1000);
    body["size"] = std::to_string(request.width) + "x" + std::to_string(request.height);

    Platform::HttpPostRequest post;
    post.url = JoinUrl(config.baseUrl, "/v1/videos/generations");
    post.body = body.dump();
    post.timeoutMs = 180000;
    post.cancel = cancel;
    post.headers.push_back(Bearer(config.apiKey));
    auto posted = http.Post(post);
    if (!posted.IsOk()) {
        return Core::Result<VideoGenResult>::Fail(posted.GetError());
    }
    auto json = ParseObject(posted.Value());
    if (!json.IsOk()) {
        return Core::Result<VideoGenResult>::Fail(json.GetError());
    }

    nlohmann::json payload = json.Value();
    std::string status;
    if (payload.contains("status") && payload["status"].is_string()) {
        status = payload["status"].get<std::string>();
    }
    std::string taskId;
    if (payload.contains("id") && payload["id"].is_string()) {
        taskId = payload["id"].get<std::string>();
    }
    int polls = 0;
    while ((status == "queued" || status == "in_progress" || status == "processing") &&
           !taskId.empty() && polls < 90) {
        if (cancel != nullptr && cancel->load()) {
            return Core::Result<VideoGenResult>::Fail(
                Fail(Core::ErrorCode::Cancelled, "cancelled", "已取消生成"));
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(2000));
        Platform::HttpGetRequest get;
        get.url = JoinUrl(config.baseUrl, "/v1/videos/" + taskId);
        get.timeoutMs = 30000;
        get.cancel = cancel;
        get.headers.push_back(Bearer(config.apiKey));
        auto polled = http.Get(get);
        if (!polled.IsOk()) {
            return Core::Result<VideoGenResult>::Fail(polled.GetError());
        }
        auto polledJson = ParseObject(polled.Value());
        if (!polledJson.IsOk()) {
            return Core::Result<VideoGenResult>::Fail(polledJson.GetError());
        }
        payload = polledJson.Value();
        status = payload.contains("status") && payload["status"].is_string()
                     ? payload["status"].get<std::string>()
                     : std::string();
        ++polls;
    }

    VideoGenResult result;
    result.jobId = taskId.empty() ? ("vid-" + request.shot.shotId) : taskId;
    result.width = request.width;
    result.height = request.height;
    result.durationMs = request.durationMs;
    result.frameRate = request.frameRate;
    result.shot = request.shot;
    result.references = request.references;
    result.outputPath = JobFile(config.outputDirectory, request.shot.shotId, "-ai.mp4");

    const std::string url = FirstDataUrl(payload);
    if (url.empty()) {
        return Core::Result<VideoGenResult>::Fail(
            Fail(Core::ErrorCode::ParseFailure, "no video payload", "生成服务没有返回视频"));
    }
    auto downloaded = DownloadHttps(http, url, result.outputPath, config.apiKey, cancel);
    if (!downloaded.IsOk()) {
        return Core::Result<VideoGenResult>::Fail(downloaded.GetError());
    }
    return Core::Result<VideoGenResult>::Ok(std::move(result));
}

std::string ExtractJsonObject(const std::string& text) {
    std::string body = text;
    const auto fence = body.find("```");
    if (fence != std::string::npos) {
        auto start = body.find('{', fence);
        auto end = body.rfind('}');
        if (start != std::string::npos && end != std::string::npos && end > start) {
            return body.substr(start, end - start + 1);
        }
    }
    const auto start = body.find('{');
    const auto end = body.rfind('}');
    if (start == std::string::npos || end == std::string::npos || end <= start) {
        return {};
    }
    return body.substr(start, end - start + 1);
}

std::string ChatMessageContent(const nlohmann::json& json) {
    if (json.contains("choices") && json["choices"].is_array() && !json["choices"].empty()) {
        const auto& choice = json["choices"][0];
        if (choice.contains("message") && choice["message"].is_object() &&
            choice["message"].contains("content") && choice["message"]["content"].is_string()) {
            return choice["message"]["content"].get<std::string>();
        }
        if (choice.contains("text") && choice["text"].is_string()) {
            return choice["text"].get<std::string>();
        }
    }
    if (json.contains("output_text") && json["output_text"].is_string()) {
        return json["output_text"].get<std::string>();
    }
    return {};
}

Core::Result<void> WriteStoryboardFile(const nlohmann::json& storyboard, const std::string& path) {
    if (!storyboard.is_object() || !storyboard.contains("format") ||
        !storyboard["format"].is_string() ||
        storyboard["format"].get<std::string>() != "DirectorDeskStoryboardImport") {
        return Core::Result<void>::Fail(
            Fail(Core::ErrorCode::ParseFailure, "bad storyboard format",
                 "模型没有返回 DirectorDesk 分镜 JSON"));
    }
    if (!storyboard.contains("formatVersion") || !storyboard["formatVersion"].is_number_integer() ||
        storyboard["formatVersion"].get<int>() != 1) {
        return Core::Result<void>::Fail(
            Fail(Core::ErrorCode::Unsupported, "storyboard version", "不支持的分镜导入版本"));
    }
    if (!storyboard.contains("scenes") || !storyboard["scenes"].is_array()) {
        return Core::Result<void>::Fail(
            Fail(Core::ErrorCode::ParseFailure, "missing scenes", "分镜 JSON 缺少 scenes"));
    }
    const std::string parent = Platform::Paths::Parent(path);
    auto created = EnsureOutputDir(parent);
    if (!created.IsOk()) {
        return created;
    }
    return Platform::Paths::WriteTextFile(path, storyboard.dump(2));
}

Core::Result<std::string> ExecuteStoryboardChat(Platform::IHttpClient& http,
                                                const HttpAiConfig& config,
                                                const StoryboardChatRequest& request,
                                                const std::atomic<bool>* cancel) {
    if (config.apiKey.empty()) {
        return Core::Result<std::string>::Fail(
            Fail(Core::ErrorCode::NotInitialized, "missing api key", "请先填写 API 密钥"));
    }
    if (config.baseUrl.rfind("https://", 0) != 0) {
        return Core::Result<std::string>::Fail(
            Fail(Core::ErrorCode::InvalidArgument, "base url must be https", "API 地址必须是 HTTPS"));
    }
    if (request.outputPath.empty() || request.outputPath.find("://") != std::string::npos) {
        return Core::Result<std::string>::Fail(
            Fail(Core::ErrorCode::InvalidArgument, "bad skill output path", "Skill 输出路径不合法"));
    }

    nlohmann::json body;
    body["model"] = config.chatModel.empty() ? "gpt-4o-mini" : config.chatModel;
    body["temperature"] = 0.2;
    nlohmann::json messages = nlohmann::json::array();
    nlohmann::json system;
    system["role"] = "system";
    system["content"] = request.systemPrompt.empty()
                            ? "你是分镜编剧。只输出 JSON，不要 Markdown。"
                            : request.systemPrompt;
    messages.push_back(std::move(system));
    nlohmann::json user;
    user["role"] = "user";
    std::string userText = request.scriptText;
    if (userText.empty()) {
        userText = "（当前剧本为空。）项目：" + request.projectName;
    } else if (!request.projectName.empty()) {
        userText = "项目：" + request.projectName + "\n\n" + userText;
    }
    user["content"] = userText;
    messages.push_back(std::move(user));
    body["messages"] = std::move(messages);
    body["response_format"] = nlohmann::json::object({{"type", "json_object"}});

    Platform::HttpPostRequest post;
    post.url = JoinUrl(config.baseUrl, "/v1/chat/completions");
    post.body = body.dump();
    post.timeoutMs = 180000;
    post.cancel = cancel;
    post.headers.push_back(Bearer(config.apiKey));
    auto posted = http.Post(post);
    if (!posted.IsOk()) {
        return Core::Result<std::string>::Fail(posted.GetError());
    }
    auto json = ParseObject(posted.Value());
    if (!json.IsOk()) {
        return Core::Result<std::string>::Fail(json.GetError());
    }
    const std::string content = ChatMessageContent(json.Value());
    const std::string extracted = ExtractJsonObject(content);
    if (extracted.empty()) {
        return Core::Result<std::string>::Fail(
            Fail(Core::ErrorCode::ParseFailure, "no json in chat", "模型没有返回 JSON"));
    }
    nlohmann::json storyboard;
    try {
        storyboard = nlohmann::json::parse(extracted);
    } catch (const nlohmann::json::exception& ex) {
        return Core::Result<std::string>::Fail(
            Fail(Core::ErrorCode::ParseFailure, ex.what(), "模型返回的 JSON 无法解析"));
    }
    auto written = WriteStoryboardFile(storyboard, request.outputPath);
    if (!written.IsOk()) {
        return Core::Result<std::string>::Fail(written.GetError());
    }
    return Core::Result<std::string>::Ok(request.outputPath);
}

Core::Result<std::string> ExecuteMockStoryboardJson(const StoryboardChatRequest& request) {
    if (request.outputPath.empty() || request.outputPath.find("://") != std::string::npos) {
        return Core::Result<std::string>::Fail(
            Fail(Core::ErrorCode::InvalidArgument, "bad skill output path", "Skill 输出路径不合法"));
    }
    nlohmann::json root;
    root["format"] = "DirectorDeskStoryboardImport";
    root["formatVersion"] = 1;
    root["source"] = nlohmann::json::object({{"tool", "directordesk/mock-storyboard-skill"},
                                             {"version", "0.6.0"}});
    root["title"] = request.projectName.empty() ? "示例分镜" : request.projectName;
    nlohmann::json shot;
    shot["id"] = "shot-skill-001";
    shot["title"] = "开场";
    shot["body"] = request.scriptText.empty() ? "角色站在咖啡馆门口。" : request.scriptText.substr(0, 160);
    shot["meta"] = nlohmann::json::object({{"景别", "全景"}, {"运镜", "固定"}, {"时长", "3s"}});
    nlohmann::json scene;
    scene["id"] = "scene-skill-demo";
    scene["title"] = "Skill 演示场";
    scene["body"] = "由本地模拟分镜 Skill 生成。";
    scene["shots"] = nlohmann::json::array({shot});
    root["scenes"] = nlohmann::json::array({scene});
    auto written = WriteStoryboardFile(root, request.outputPath);
    if (!written.IsOk()) {
        return Core::Result<std::string>::Fail(written.GetError());
    }
    return Core::Result<std::string>::Ok(request.outputPath);
}

} // namespace DirectorDesk::AI
