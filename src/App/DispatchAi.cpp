// DispatchAi: Generate image/video, save AI settings, run Skill (F5).
#include "DirectorDesk/App/CommandDispatch.h"

#include "AppInternals.h"
#include "DirectorDesk/AI/HttpCompatible.h"
#include "DirectorDesk/AI/SkillRunner.h"
#include "DirectorDesk/App/UserSettings.h"
#include "DirectorDesk/Export/ShotExport.h"
#include "DirectorDesk/Platform/Paths.h"
#include "DirectorDesk/Platform/Worker.h"

#include <memory>
#include <variant>

namespace DirectorDesk::App {
namespace {

const Script::Shot* FindShotById(const Script::Document& script, const std::string& shotId) {
    if (!script.HasPublishedSnapshot()) {
        return nullptr;
    }
    for (const Script::Scene& scene : script.PublishedSnapshot().scenes) {
        for (const Script::Shot& shot : scene.shots) {
            if (shot.id == shotId) {
                return &shot;
            }
        }
    }
    return nullptr;
}

std::string MetaValue(const Script::Shot& shot, const char* key) {
    for (const Script::ShotMeta& item : shot.meta) {
        if (item.key == key) {
            return item.value;
        }
    }
    return {};
}

std::string PromptFor(const Script::Shot& shot) {
    const std::string prompt = MetaValue(shot, "提示词");
    if (!prompt.empty()) {
        return prompt;
    }
    if (!shot.body.empty()) {
        return shot.body;
    }
    return shot.title;
}

std::string AiOutputDirectory(const AppState& state) {
    if (!state.userSettingsPath.empty()) {
        return Platform::Paths::Join(Platform::Paths::Parent(state.userSettingsPath), "ai-output");
    }
    auto temp = Platform::Paths::TemporaryDirectory();
    if (temp.IsOk()) {
        return Platform::Paths::Join(temp.Value(), "DirectorDesk-ai-output");
    }
    return "ai-output";
}

void SaveAiSettingsFile(AppState& state) {
    if (state.userSettingsPath.empty()) {
        return;
    }
    auto saved = SaveUserSettings(state.userSettingsPath, state.userSettings);
    if (!saved.IsOk()) {
        state.status = saved.GetError().userMessage;
    }
}

void FinishLocalAi(AppState& state, const std::string& kind, const std::string& shotId,
                   const std::string& path, bool ok, const std::string& message) {
    state.aiBusy = false;
    state.aiJobStatus = ok ? "succeeded" : "failed";
    state.aiJobMessage = message;
    state.aiJobRatio = ok ? 1.0f : 0.0f;
    UI::ExportLogView log;
    log.label = kind == "video" ? "ai-video" : "ai-image";
    log.shotId = shotId;
    log.shotTitle = FindShotTitle(state.script, shotId);
    log.path = path;
    log.ok = ok;
    if (ok) {
        state.lastAiOutputPath = path;
        state.status = (kind == "video" ? "已生成视频 " : "已生成图像 ") +
                       Platform::Paths::FileName(path);
        log.message = state.status;
    } else {
        state.status = message;
        log.message = message;
    }
    PushExportLog(state.exportLog, std::move(log));
}

bool StartGeneration(AppState& state, DispatchServices& services, const std::string& requestedShotId,
                     bool video) {
    if (state.aiBusy) {
        state.status = "已有生成任务";
        return true;
    }
    std::string shotId = requestedShotId;
    if (shotId.empty()) {
        shotId = state.script.SelectedShotId();
    }
    const Script::Shot* shot = FindShotById(state.script, shotId);
    if (shot == nullptr) {
        state.status = "没有可生成的镜头";
        return true;
    }
    Export::ShotResolution resolution = Export::ShotResolution::Hd1080;
    if (!Export::TryParseResolution(state.exportResolutionId, resolution)) {
        resolution = Export::ShotResolution::Hd1080;
    }
    const Export::ShotSize size = Export::SizeFor(resolution);
    const std::string provider = AI::NormalizeAiProvider(state.userSettings.aiProvider);
    const std::string outputDir = AiOutputDirectory(state);

    if (provider == "mock") {
        if (video) {
            AI::VideoGenRequest request;
            request.prompt = PromptFor(*shot);
            request.shot.shotId = shot->id;
            request.shot.shotTitle = shot->title;
            request.shot.shotText = shot->body;
            request.shot.projectName = state.projectName;
            request.width = size.width;
            request.height = size.height;
            request.durationMs = 4000;
            request.frameRate = 24;
            auto result = AI::ExecuteMockVideo(request, outputDir);
            FinishLocalAi(state, "video", shotId, result.IsOk() ? result.Value().outputPath : "",
                          result.IsOk(), result.IsOk() ? "" : result.GetError().userMessage);
        } else {
            AI::ImageGenRequest request;
            request.prompt = PromptFor(*shot);
            request.shot.shotId = shot->id;
            request.shot.shotTitle = shot->title;
            request.shot.shotText = shot->body;
            request.shot.projectName = state.projectName;
            request.width = size.width;
            request.height = size.height;
            auto result = AI::ExecuteMockImage(request, outputDir);
            FinishLocalAi(state, "image", shotId, result.IsOk() ? result.Value().outputPath : "",
                          result.IsOk(), result.IsOk() ? "" : result.GetError().userMessage);
        }
        return true;
    }

    if (state.userSettings.aiApiKey.empty()) {
        state.status = "请先填写 API 密钥";
        return true;
    }
    if (services.worker == nullptr || services.http == nullptr || services.aiResults == nullptr) {
        state.status = "生成服务不可用";
        return true;
    }

    AI::HttpAiConfig config;
    config.baseUrl = state.userSettings.aiBaseUrl.empty() ? "https://api.openai.com"
                                                          : state.userSettings.aiBaseUrl;
    config.apiKey = state.userSettings.aiApiKey;
    config.imageModel = state.userSettings.aiImageModel;
    config.videoModel = state.userSettings.aiVideoModel;
    config.outputDirectory = outputDir;

    auto cancel = std::make_shared<std::atomic<bool>>(false);
    state.aiCancel = cancel;
    state.aiBusy = true;
    state.aiJobKind = video ? "video" : "image";
    state.aiJobShotId = shotId;
    state.aiJobStatus = "queued";
    state.aiJobMessage = "排队中";
    state.aiJobRatio = 0.0f;
    state.status = video ? "正在生成视频…" : "正在生成图像…";

    Platform::IHttpClient* http = services.http;
    Core::ResultQueue<AiJobResult>* results = services.aiResults;
    const std::string prompt = PromptFor(*shot);
    const std::string title = shot->title;
    const std::string body = shot->body;
    const std::string projectName = state.projectName;
    services.worker->Submit([http, config, cancel, results, video, shotId, prompt, title, body,
                             projectName, size]() {
        AiJobResult job;
        job.kind = video ? "video" : "image";
        job.shotId = shotId;
        job.jobId = shotId;
        if (video) {
            AI::VideoGenRequest request;
            request.prompt = prompt;
            request.shot.shotId = shotId;
            request.shot.shotTitle = title;
            request.shot.shotText = body;
            request.shot.projectName = projectName;
            request.width = size.width;
            request.height = size.height;
            request.durationMs = 4000;
            request.frameRate = 24;
            auto result = AI::ExecuteVideoGeneration(*http, config, request, cancel.get());
            job.ok = result.IsOk();
            if (result.IsOk()) {
                job.outputPath = result.Value().outputPath;
                job.jobId = result.Value().jobId;
            } else {
                job.message = result.GetError().userMessage;
            }
        } else {
            AI::ImageGenRequest request;
            request.prompt = prompt;
            request.shot.shotId = shotId;
            request.shot.shotTitle = title;
            request.shot.shotText = body;
            request.shot.projectName = projectName;
            request.width = size.width;
            request.height = size.height;
            auto result = AI::ExecuteImageGeneration(*http, config, request, cancel.get());
            job.ok = result.IsOk();
            if (result.IsOk()) {
                job.outputPath = result.Value().outputPath;
                job.jobId = result.Value().jobId;
            } else {
                job.message = result.GetError().userMessage;
            }
        }
        results->Push(std::move(job));
    });
    return true;
}

} // namespace

void DrainAiResults(AppState& state, Core::ResultQueue<AiJobResult>& aiResults) {
    AiJobResult job;
    while (aiResults.TryPop(job)) {
        state.aiBusy = false;
        state.aiCancel.reset();
        state.aiJobId = job.jobId;
        state.aiJobStatus = job.ok ? "succeeded" : "failed";
        state.aiJobMessage = job.message;
        state.aiJobRatio = job.ok ? 1.0f : 0.0f;
        UI::ExportLogView log;
        log.label = job.kind == "video" ? "ai-video" : (job.kind == "skill" ? "ai-skill" : "ai-image");
        log.shotId = job.shotId;
        log.shotTitle = FindShotTitle(state.script, job.shotId);
        log.path = job.outputPath;
        log.ok = job.ok;
        if (job.ok) {
            state.lastAiOutputPath = job.outputPath;
            if (job.kind == "skill") {
                state.status = "已运行 Skill";
            } else {
                state.status = (job.kind == "video" ? "已生成视频 " : "已生成图像 ") +
                               Platform::Paths::FileName(job.outputPath);
            }
            log.message = state.status;
        } else {
            state.status = job.message;
            log.message = job.message;
        }
        PushExportLog(state.exportLog, std::move(log));
        if (job.ok && job.kind == "skill" && !job.importJsonPath.empty()) {
            ApplyStoryboardImport(state, job.importJsonPath, "append");
        }
    }
}

bool TryDispatchAi(AppState& state, const Core::Command& command, DispatchServices& services) {
    if (const auto* typed = std::get_if<Core::SetAiSettingsCommand>(&command)) {
        state.userSettings.aiProvider = AI::NormalizeAiProvider(typed->provider);
        if (!typed->baseUrl.empty()) {
            state.userSettings.aiBaseUrl = typed->baseUrl;
        }
        if (!typed->apiKey.empty()) {
            state.userSettings.aiApiKey = typed->apiKey;
        }
        if (!typed->imageModel.empty()) {
            state.userSettings.aiImageModel = typed->imageModel;
        }
        if (!typed->videoModel.empty()) {
            state.userSettings.aiVideoModel = typed->videoModel;
        }
        if (!typed->chatModel.empty()) {
            state.userSettings.aiChatModel = typed->chatModel;
        }
        SaveAiSettingsFile(state);
        if (state.status.find("设置路径") == std::string::npos) {
            state.status = "已保存 AI 设置";
        }
        return true;
    }
    if (const auto* typed = std::get_if<Core::GenerateShotImageCommand>(&command)) {
        return StartGeneration(state, services, typed->shotId, false);
    }
    if (const auto* typed = std::get_if<Core::GenerateShotVideoCommand>(&command)) {
        return StartGeneration(state, services, typed->shotId, true);
    }
    if (std::holds_alternative<Core::CancelAiJobCommand>(command)) {
        if (!state.aiBusy) {
            return true;
        }
        if (state.aiCancel) {
            state.aiCancel->store(true);
        }
        state.aiBusy = false;
        state.aiJobStatus = "cancelled";
        state.aiJobMessage = "已取消生成";
        state.status = "已取消生成";
        return true;
    }
    if (const auto* typed = std::get_if<Core::RunSkillCommand>(&command)) {
        std::string assetId = typed->assetId;
        if (assetId.empty()) {
            assetId = state.selectedLibraryAssetId;
        }
        const Asset::LibraryAsset* asset = state.library.Find(assetId);
        std::string skillDir;
        if (asset != nullptr && asset->format == "skill") {
            skillDir = Platform::Paths::Parent(asset->sourcePath);
        } else if (const Asset::OfficialAssetState* official = state.officialCatalog.State(assetId)) {
            if (!official->entrypointPath.empty()) {
                skillDir = Platform::Paths::Parent(official->entrypointPath);
            }
        }
        if (skillDir.empty()) {
            state.status = "请先选中已安装的 Skill";
            return true;
        }
        if (state.aiBusy) {
            state.status = "已有生成任务";
            return true;
        }
        AI::SkillRunRequest request;
        request.skillDirectory = skillDir;
        request.outputDirectory = AiOutputDirectory(state);
        request.scriptText = state.script.Text();
        request.projectName = state.projectName;
        request.provider = AI::NormalizeAiProvider(state.userSettings.aiProvider);
        request.config.baseUrl = state.userSettings.aiBaseUrl.empty() ? "https://api.openai.com"
                                                                       : state.userSettings.aiBaseUrl;
        request.config.apiKey = state.userSettings.aiApiKey;
        request.config.imageModel = state.userSettings.aiImageModel;
        request.config.videoModel = state.userSettings.aiVideoModel;
        request.config.chatModel = state.userSettings.aiChatModel.empty() ? "gpt-4o-mini"
                                                                         : state.userSettings.aiChatModel;
        request.config.outputDirectory = request.outputDirectory;
        const std::string builtin = AI::SkillBuiltin(skillDir);
        const bool usesLlm = builtin == "openai-compat-json";
        if (usesLlm && request.provider != "mock" && state.userSettings.aiApiKey.empty()) {
            state.status = "请先填写 API 密钥";
            return true;
        }
        if (usesLlm && request.provider != "mock") {
            if (services.worker == nullptr || services.http == nullptr ||
                services.aiResults == nullptr) {
                state.status = "生成服务不可用";
                return true;
            }
            auto cancel = std::make_shared<std::atomic<bool>>(false);
            state.aiCancel = cancel;
            state.aiBusy = true;
            state.aiJobKind = "skill";
            state.aiJobStatus = "queued";
            state.aiJobMessage = "正在运行分镜 Skill…";
            state.aiJobRatio = 0.0f;
            state.status = "正在运行分镜 Skill…";
            Platform::IHttpClient* http = services.http;
            Core::ResultQueue<AiJobResult>* results = services.aiResults;
            request.http = http;
            request.cancel = cancel.get();
            services.worker->Submit([request, cancel, results]() {
                AiJobResult job;
                job.kind = "skill";
                job.jobId = "skill";
                AI::SkillRunRequest local = request;
                local.cancel = cancel.get();
                auto ran = AI::RunSkill(local);
                job.ok = ran.IsOk();
                if (ran.IsOk()) {
                    job.outputPath = ran.Value().outputJsonPath;
                    job.importJsonPath = ran.Value().outputJsonPath;
                } else {
                    job.message = ran.GetError().userMessage;
                }
                results->Push(std::move(job));
            });
            return true;
        }
        auto ran = AI::RunSkill(request);
        if (!ran.IsOk()) {
            state.status = ran.GetError().userMessage;
            return true;
        }
        ApplyStoryboardImport(state, ran.Value().outputJsonPath, "append");
        if (state.status.find("已导入") == std::string::npos) {
            state.status = "已运行 Skill 并导入";
        }
        return true;
    }
    return false;
}

} // namespace DirectorDesk::App
