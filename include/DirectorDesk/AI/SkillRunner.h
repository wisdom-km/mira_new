// SkillRunner: Run an installed Skill via skill.json (copy-output, LLM JSON, or subprocess).

#pragma once

#include "DirectorDesk/AI/HttpCompatible.h"
#include "DirectorDesk/Core/Result.h"
#include "DirectorDesk/Platform/IHttpClient.h"

#include <atomic>
#include <string>

namespace DirectorDesk::AI {

struct SkillRunRequest {
    std::string skillDirectory;
    std::string outputDirectory;
    std::string scriptText;
    std::string projectName;
    std::string provider;
    Platform::IHttpClient* http = nullptr;
    HttpAiConfig config;
    const std::atomic<bool>* cancel = nullptr;
};

struct SkillRunResult {
    std::string outputJsonPath;
    bool usedNetwork = false;
};

[[nodiscard]] std::string SkillBuiltin(const std::string& skillDirectory);

Core::Result<SkillRunResult> RunSkill(const SkillRunRequest& request);

} // namespace DirectorDesk::AI
