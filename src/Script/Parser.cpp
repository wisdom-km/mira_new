#include "DirectorDesk/Script/Parser.h"

#include "DirectorDesk/Script/Ids.h"

#include <cstdint>
#include <cctype>
#include <set>
#include <utility>

namespace DirectorDesk::Script {
namespace {

struct Line {
    int number = 1;
    std::string text;
};

std::string Trim(const std::string& value) {
    std::size_t begin = 0;
    while (begin < value.size() &&
           std::isspace(static_cast<unsigned char>(value[begin])) != 0) {
        ++begin;
    }
    std::size_t end = value.size();
    while (end > begin && std::isspace(static_cast<unsigned char>(value[end - 1])) != 0) {
        --end;
    }
    return value.substr(begin, end - begin);
}

bool IsValidUtf8(const std::string& text) {
    const auto* bytes = reinterpret_cast<const unsigned char*>(text.data());
    const std::size_t size = text.size();
    for (std::size_t i = 0; i < size;) {
        const unsigned char lead = bytes[i];
        std::size_t extra = 0;
        std::uint32_t codepoint = 0;
        if (lead <= 0x7Fu) {
            ++i;
            continue;
        }
        if ((lead & 0xE0u) == 0xC0u) {
            extra = 1;
            codepoint = lead & 0x1Fu;
        } else if ((lead & 0xF0u) == 0xE0u) {
            extra = 2;
            codepoint = lead & 0x0Fu;
        } else if ((lead & 0xF8u) == 0xF0u) {
            extra = 3;
            codepoint = lead & 0x07u;
        } else {
            return false;
        }
        if (i + extra >= size) {
            return false;
        }
        for (std::size_t n = 1; n <= extra; ++n) {
            if ((bytes[i + n] & 0xC0u) != 0x80u) {
                return false;
            }
            codepoint = (codepoint << 6u) | (bytes[i + n] & 0x3Fu);
        }
        if ((extra == 1 && codepoint < 0x80u) || (extra == 2 && codepoint < 0x800u) ||
            (extra == 3 && codepoint < 0x10000u) || codepoint > 0x10FFFFu ||
            (codepoint >= 0xD800u && codepoint <= 0xDFFFu)) {
            return false;
        }
        i += extra + 1;
    }
    return true;
}

std::string StripBom(const std::string& text) {
    if (text.size() >= 3 && static_cast<unsigned char>(text[0]) == 0xef &&
        static_cast<unsigned char>(text[1]) == 0xbb &&
        static_cast<unsigned char>(text[2]) == 0xbf) {
        return text.substr(3);
    }
    return text;
}

std::vector<Line> SplitLines(const std::string& text) {
    std::vector<Line> lines;
    int number = 1;
    std::size_t i = 0;
    while (i < text.size()) {
        const std::size_t start = i;
        while (i < text.size() && text[i] != '\n' && text[i] != '\r') {
            ++i;
        }
        lines.push_back(Line{number++, text.substr(start, i - start)});
        if (i < text.size() && text[i] == '\r') {
            ++i;
        }
        if (i < text.size() && text[i] == '\n') {
            ++i;
        }
    }
    return lines;
}

bool StartsWithFence(const std::string& line, char& fenceChar, int& fenceLen) {
    std::size_t i = 0;
    while (i < line.size() && i < 3 && line[i] == ' ') {
        ++i;
    }
    if (i >= line.size()) {
        return false;
    }
    const char marker = line[i];
    if (marker != '`' && marker != '~') {
        return false;
    }
    int count = 0;
    while (i < line.size() && line[i] == marker) {
        ++count;
        ++i;
    }
    if (count < 3) {
        return false;
    }
    fenceChar = marker;
    fenceLen = count;
    return true;
}

bool ParseAtxHeading(const std::string& line, int& level, std::string& content) {
    std::size_t i = 0;
    while (i < line.size() && i < 3 && line[i] == ' ') {
        ++i;
    }
    int hashes = 0;
    while (i < line.size() && line[i] == '#') {
        ++hashes;
        ++i;
    }
    if (hashes < 1 || hashes > 6) {
        return false;
    }
    if (i < line.size() && line[i] != ' ' && line[i] != '\t') {
        return false;
    }
    if (i < line.size()) {
        ++i;
    }
    content = line.substr(i);
    level = hashes;
    return true;
}

bool TryParseMarker(const std::string& content, const char* kind, std::string& id,
                    std::string& title) {
    const std::string prefix = std::string("[") + kind + ":";
    if (content.size() < prefix.size() || content.compare(0, prefix.size(), prefix) != 0) {
        return false;
    }
    const std::size_t close = content.find(']', prefix.size());
    if (close == std::string::npos) {
        return false;
    }
    id = content.substr(prefix.size(), close - prefix.size());
    title = Trim(content.substr(close + 1));
    return true;
}

void AddDiagnostic(ParseResult& result, DiagnosticSeverity severity, int line, const char* code,
                   std::string message) {
    Diagnostic diagnostic;
    diagnostic.severity = severity;
    diagnostic.line = line;
    diagnostic.code = code;
    diagnostic.message = std::move(message);
    result.diagnostics.push_back(std::move(diagnostic));
}

void FlushShot(ParseResult& result, Scene* scene, Shot& shot, bool& hasShot) {
    if (!hasShot || scene == nullptr) {
        return;
    }
    if (Trim(shot.body).empty()) {
        AddDiagnostic(result, DiagnosticSeverity::Hint, shot.headingLine, "script.empty-shot-body",
                      "镜头正文为空");
    }
    scene->shots.push_back(std::move(shot));
    shot = Shot{};
    hasShot = false;
}

} // namespace

ParseResult Parser::Parse(const std::string& markdown) {
    ParseResult result;
    if (!IsValidUtf8(markdown)) {
        result.utf8Valid = false;
        result.completed = false;
        AddDiagnostic(result, DiagnosticSeverity::Error, 1, "script.invalid-utf8",
                      "文件不是合法 UTF-8，已拒绝加载");
        return result;
    }

    const std::string text = StripBom(markdown);
    const std::vector<Line> lines = SplitLines(text);
    std::set<std::string> sceneIds;
    std::set<std::string> shotIds;
    Scene* currentScene = nullptr;
    Shot currentShot;
    bool hasShot = false;
    bool inFence = false;
    char fenceChar = '`';
    int fenceLen = 0;
    std::string* bodyTarget = nullptr;

    auto endCurrentBodies = [&]() {
        FlushShot(result, currentScene, currentShot, hasShot);
        bodyTarget = currentScene == nullptr ? nullptr : &currentScene->body;
    };

    for (const Line& line : lines) {
        char nextFenceChar = '`';
        int nextFenceLen = 0;
        if (StartsWithFence(line.text, nextFenceChar, nextFenceLen)) {
            if (!inFence) {
                inFence = true;
                fenceChar = nextFenceChar;
                fenceLen = nextFenceLen;
            } else if (nextFenceChar == fenceChar && nextFenceLen >= fenceLen) {
                inFence = false;
            }
            if (bodyTarget != nullptr) {
                if (!bodyTarget->empty()) {
                    *bodyTarget += '\n';
                }
                *bodyTarget += line.text;
            }
            continue;
        }

        int level = 0;
        std::string heading;
        if (!inFence && ParseAtxHeading(line.text, level, heading)) {
            if (level == 1) {
                if (result.snapshot.documentTitle.empty()) {
                    const std::string title = Trim(heading);
                    if (!title.empty()) {
                        result.snapshot.documentTitle = title;
                    }
                }
                endCurrentBodies();
                continue;
            }

            if (level == 2) {
                endCurrentBodies();
                std::string id;
                std::string title;
                if (!TryParseMarker(heading, "scene", id, title)) {
                    AddDiagnostic(result, DiagnosticSeverity::Hint, line.number,
                                  "script.unmarked-heading",
                                  "二级标题缺少 [scene:<id>] 标记，已视为普通标题");
                    bodyTarget = nullptr;
                    continue;
                }
                if (!IsValidId(id)) {
                    AddDiagnostic(result, DiagnosticSeverity::Error, line.number,
                                  "script.invalid-id", "场次 ID 不合法，未创建节点");
                    bodyTarget = nullptr;
                    continue;
                }
                if (sceneIds.count(id) != 0) {
                    AddDiagnostic(result, DiagnosticSeverity::Error, line.number,
                                  "script.duplicate-id", "场次 ID 重复，已保留第一次出现");
                    bodyTarget = nullptr;
                    continue;
                }
                sceneIds.insert(id);
                Scene scene;
                scene.id = std::move(id);
                scene.headingLine = line.number;
                if (title.empty()) {
                    scene.title = "未命名";
                    AddDiagnostic(result, DiagnosticSeverity::Warning, line.number,
                                  "script.empty-title", "场次标题为空，已显示为“未命名”");
                } else {
                    scene.title = std::move(title);
                }
                result.snapshot.scenes.push_back(std::move(scene));
                currentScene = &result.snapshot.scenes.back();
                bodyTarget = &currentScene->body;
                continue;
            }

            if (level == 3) {
                FlushShot(result, currentScene, currentShot, hasShot);
                std::string id;
                std::string title;
                if (!TryParseMarker(heading, "shot", id, title)) {
                    AddDiagnostic(result, DiagnosticSeverity::Hint, line.number,
                                  "script.unmarked-heading",
                                  "三级标题缺少 [shot:<id>] 标记，已视为普通标题");
                    bodyTarget = currentScene == nullptr ? nullptr : &currentScene->body;
                    continue;
                }
                if (currentScene == nullptr) {
                    AddDiagnostic(result, DiagnosticSeverity::Error, line.number,
                                  "script.shot-before-scene", "镜头出现在场次之前，未创建该镜头");
                    bodyTarget = nullptr;
                    continue;
                }
                if (!IsValidId(id)) {
                    AddDiagnostic(result, DiagnosticSeverity::Error, line.number,
                                  "script.invalid-id", "镜头 ID 不合法，未创建节点");
                    bodyTarget = &currentScene->body;
                    continue;
                }
                if (shotIds.count(id) != 0) {
                    AddDiagnostic(result, DiagnosticSeverity::Error, line.number,
                                  "script.duplicate-id", "镜头 ID 重复，已保留第一次出现");
                    bodyTarget = &currentScene->body;
                    continue;
                }
                shotIds.insert(id);
                currentShot = Shot{};
                currentShot.id = std::move(id);
                currentShot.headingLine = line.number;
                if (title.empty()) {
                    currentShot.title = "未命名";
                    AddDiagnostic(result, DiagnosticSeverity::Warning, line.number,
                                  "script.empty-title", "镜头标题为空，已显示为“未命名”");
                } else {
                    currentShot.title = std::move(title);
                }
                hasShot = true;
                bodyTarget = &currentShot.body;
                continue;
            }
        }

        if (bodyTarget != nullptr) {
            if (!bodyTarget->empty()) {
                *bodyTarget += '\n';
            }
            *bodyTarget += line.text;
        }
    }

    FlushShot(result, currentScene, currentShot, hasShot);
    result.completed = true;
    result.utf8Valid = true;
    return result;
}

} // namespace DirectorDesk::Script
