// SkillRunner: Implementation for installed Skill folders.

#include "DirectorDesk/AI/SkillRunner.h"

#include "DirectorDesk/Core/Error.h"
#include "DirectorDesk/Platform/Paths.h"
#include "DirectorDesk/Platform/Process.h"

#include <nlohmann/json.hpp>

namespace DirectorDesk::AI {
namespace {

Core::Error Fail(Core::ErrorCode code, const std::string& technical, const std::string& user) {
    return Core::Error::Make(code, technical, user);
}

bool IsSafeLeaf(const std::string& name) {
    return !name.empty() && name.find("..") == std::string::npos && name.find(':') == std::string::npos &&
           name.find('/') == std::string::npos && name.find('\\') == std::string::npos;
}

const char* kDefaultStoryboardPrompt =
    "你是 DirectorDesk 的分镜编剧。根据用户给出的剧本文本，只输出一个 JSON 对象，不要 Markdown、不要解释。"
    "JSON 必须符合：format=DirectorDeskStoryboardImport，formatVersion=1，"
    "scenes[].id/title/body，scenes[].shots[].id/title/body，shots[].meta 可用键：景别、运镜、时长、提示词。"
    "按场次拆镜，每镜写清画面动作。";

nlohmann::json ParseSkillJson(const std::string& skillDirectory, Core::Result<void>& error) {
    const std::string descPath = Platform::Paths::Join(skillDirectory, "skill.json");
    if (!Platform::Paths::Exists(descPath)) {
        error = Core::Result<void>::Fail(
            Fail(Core::ErrorCode::NotFound, "skill.json missing", "本 Skill 需在 Agent 中运行；已安装"));
        return {};
    }
    auto text = Platform::Paths::ReadTextFile(descPath);
    if (!text.IsOk()) {
        error = Core::Result<void>::Fail(text.GetError());
        return {};
    }
    try {
        auto root = nlohmann::json::parse(text.Value());
        if (!root.is_object() || !root.contains("format") || !root["format"].is_string() ||
            root["format"].get<std::string>() != "DirectorDeskSkillRun") {
            error = Core::Result<void>::Fail(Fail(Core::ErrorCode::InvalidArgument, "bad skill format",
                                                  "不是 DirectorDesk Skill 运行描述"));
            return {};
        }
        if (!root.contains("formatVersion") || !root["formatVersion"].is_number_integer() ||
            root["formatVersion"].get<int>() != 1) {
            error = Core::Result<void>::Fail(
                Fail(Core::ErrorCode::Unsupported, "skill-run version", "不支持的 Skill 运行版本"));
            return {};
        }
        error = Core::Result<void>::Ok();
        return root;
    } catch (const nlohmann::json::exception& ex) {
        error = Core::Result<void>::Fail(
            Fail(Core::ErrorCode::ParseFailure, ex.what(), "skill.json 无法解析"));
        return {};
    }
}

} // namespace

std::string SkillBuiltin(const std::string& skillDirectory) {
    Core::Result<void> parsed = Core::Result<void>::Ok();
    const nlohmann::json root = ParseSkillJson(skillDirectory, parsed);
    if (!parsed.IsOk() || !root.contains("builtin") || !root["builtin"].is_string()) {
        return {};
    }
    return root["builtin"].get<std::string>();
}

Core::Result<SkillRunResult> RunSkill(const SkillRunRequest& request) {
    if (request.skillDirectory.empty() || !Platform::Paths::IsDirectory(request.skillDirectory)) {
        return Core::Result<SkillRunResult>::Fail(
            Fail(Core::ErrorCode::NotFound, "skill dir missing", "找不到 Skill 安装目录"));
    }
    Core::Result<void> parsed = Core::Result<void>::Ok();
    const nlohmann::json root = ParseSkillJson(request.skillDirectory, parsed);
    if (!parsed.IsOk()) {
        return Core::Result<SkillRunResult>::Fail(parsed.GetError());
    }

    std::string outputName = "storyboard-import.json";
    if (root.contains("output") && root["output"].is_string() &&
        !root["output"].get<std::string>().empty()) {
        outputName = root["output"].get<std::string>();
    }
    if (!IsSafeLeaf(outputName)) {
        return Core::Result<SkillRunResult>::Fail(
            Fail(Core::ErrorCode::InvalidArgument, "unsafe output name", "Skill 输出路径不合法"));
    }

    std::string builtin;
    if (root.contains("builtin") && root["builtin"].is_string()) {
        builtin = root["builtin"].get<std::string>();
    }

    if (root.contains("argv") && root["argv"].is_array() && !root["argv"].empty()) {
        Platform::ProcessRequest process;
        process.workingDirectory = request.skillDirectory;
        process.timeoutMs = 180000;
        process.cancel = request.cancel;
        for (const auto& item : root["argv"]) {
            if (item.is_string()) {
                process.argv.push_back(item.get<std::string>());
            }
        }
        auto ran = Platform::RunProcess(process);
        if (!ran.IsOk()) {
            return Core::Result<SkillRunResult>::Fail(ran.GetError());
        }
    }

    if (builtin == "openai-compat-json") {
        std::string promptFile = "prompt.md";
        if (root.contains("promptFile") && root["promptFile"].is_string() &&
            !root["promptFile"].get<std::string>().empty()) {
            promptFile = root["promptFile"].get<std::string>();
        }
        if (!IsSafeLeaf(promptFile)) {
            return Core::Result<SkillRunResult>::Fail(
                Fail(Core::ErrorCode::InvalidArgument, "unsafe prompt file", "Skill 提示词路径不合法"));
        }
        std::string systemPrompt = kDefaultStoryboardPrompt;
        const std::string promptPath = Platform::Paths::Join(request.skillDirectory, promptFile);
        if (Platform::Paths::Exists(promptPath)) {
            auto promptText = Platform::Paths::ReadTextFile(promptPath);
            if (promptText.IsOk() && !promptText.Value().empty()) {
                systemPrompt = promptText.Value();
            }
        }
        std::string dest = Platform::Paths::Join(request.skillDirectory, outputName);
        if (!request.outputDirectory.empty()) {
            auto created = Platform::Paths::CreateDirectories(request.outputDirectory);
            if (!created.IsOk()) {
                return Core::Result<SkillRunResult>::Fail(created.GetError());
            }
            dest = Platform::Paths::Join(request.outputDirectory, outputName);
        }
        StoryboardChatRequest chat;
        chat.scriptText = request.scriptText;
        chat.projectName = request.projectName;
        chat.systemPrompt = systemPrompt;
        chat.outputPath = dest;
        if (IsMockProvider(request.provider)) {
            auto written = ExecuteMockStoryboardJson(chat);
            if (!written.IsOk()) {
                return Core::Result<SkillRunResult>::Fail(written.GetError());
            }
            SkillRunResult result;
            result.outputJsonPath = written.Value();
            return Core::Result<SkillRunResult>::Ok(std::move(result));
        }
        if (request.http == nullptr) {
            return Core::Result<SkillRunResult>::Fail(
                Fail(Core::ErrorCode::NotInitialized, "missing http", "生成服务不可用"));
        }
        auto written = ExecuteStoryboardChat(*request.http, request.config, chat, request.cancel);
        if (!written.IsOk()) {
            return Core::Result<SkillRunResult>::Fail(written.GetError());
        }
        SkillRunResult result;
        result.outputJsonPath = written.Value();
        result.usedNetwork = true;
        return Core::Result<SkillRunResult>::Ok(std::move(result));
    }

    const std::string source = Platform::Paths::Join(request.skillDirectory, outputName);
    if (!Platform::Paths::Exists(source)) {
        return Core::Result<SkillRunResult>::Fail(
            Fail(Core::ErrorCode::NotFound, "skill output missing", "Skill 没有写出分镜 JSON"));
    }

    SkillRunResult result;
    if (builtin == "copy-output" || builtin.empty()) {
        if (request.outputDirectory.empty()) {
            result.outputJsonPath = source;
            return Core::Result<SkillRunResult>::Ok(std::move(result));
        }
        auto created = Platform::Paths::CreateDirectories(request.outputDirectory);
        if (!created.IsOk()) {
            return Core::Result<SkillRunResult>::Fail(created.GetError());
        }
        const std::string dest = Platform::Paths::Join(request.outputDirectory, outputName);
        auto copied = Platform::Paths::CopyFileUtf8(source, dest);
        if (!copied.IsOk()) {
            return Core::Result<SkillRunResult>::Fail(copied.GetError());
        }
        result.outputJsonPath = dest;
        return Core::Result<SkillRunResult>::Ok(std::move(result));
    }

    result.outputJsonPath = source;
    return Core::Result<SkillRunResult>::Ok(std::move(result));
}

} // namespace DirectorDesk::AI
