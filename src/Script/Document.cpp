// Document: Implementation for the DirectorDesk Script module.
// This file owns project behavior only; keep platform and dependency boundaries explicit.

#include "DirectorDesk/Script/Document.h"

#include "DirectorDesk/Core/Error.h"
#include "DirectorDesk/Platform/Paths.h"
#include "DirectorDesk/Script/Ids.h"
#include "DirectorDesk/Script/Parser.h"

#include <cctype>
#include <vector>

namespace DirectorDesk::Script {
namespace {

bool ContainsCrlf(const std::string& text) {
    return text.find("\r\n") != std::string::npos;
}

std::string NormalizeLf(const std::string& text) {
    std::string out;
    out.reserve(text.size());
    for (std::size_t i = 0; i < text.size(); ++i) {
        if (text[i] == '\r') {
            out += '\n';
            if (i + 1 < text.size() && text[i + 1] == '\n') {
                ++i;
            }
        } else {
            out += text[i];
        }
    }
    return out;
}

const Shot* FindShot(const Snapshot& snapshot, const std::string& shotId) {
    for (const Scene& scene : snapshot.scenes) {
        for (const Shot& shot : scene.shots) {
            if (shot.id == shotId) {
                return &shot;
            }
        }
    }
    return nullptr;
}

bool ShotExists(const Snapshot& snapshot, const std::string& shotId) {
    return FindShot(snapshot, shotId) != nullptr;
}

std::size_t OffsetOfLine(const std::string& text, int line) {
    if (line <= 1) {
        return 0;
    }
    int current = 1;
    for (std::size_t i = 0; i < text.size(); ++i) {
        if (text[i] == '\n') {
            ++current;
            if (current == line) {
                return i + 1;
            }
        }
    }
    return text.size();
}

std::string FirstShotIdInSnapshot(const Snapshot& snapshot) {
    for (const Scene& scene : snapshot.scenes) {
        if (!scene.shots.empty()) {
            return scene.shots.front().id;
        }
    }
    return {};
}

std::string TrimCopy(const std::string& value) {
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

} // namespace

Document::Document() {
    ApplyParse(Parser::Parse(""), false);
}

void Document::ApplyParse(const ParseResult& parsed, bool incrementRevision) {
    if (!parsed.utf8Valid || !parsed.completed) {
        return;
    }
    m_snapshot = parsed.snapshot;
    m_diagnostics = parsed.diagnostics;
    m_hasSnapshot = true;
    if (!m_selectedShotId.empty() && !ShotExists(m_snapshot, m_selectedShotId)) {
        m_selectedShotId.clear();
    }
    if (incrementRevision) {
        ++m_externalRevision;
    }
}

void Document::RememberWriteTime() {
    if (m_path.empty() || !Platform::Paths::Exists(m_path)) {
        m_hasWriteTime = false;
        m_writeTime = 0;
        m_writeSize = 0;
        return;
    }
    auto time = Platform::Paths::LastWriteTimeCount(m_path);
    auto size = Platform::Paths::FileSize(m_path);
    if (!time.IsOk() || !size.IsOk()) {
        m_hasWriteTime = false;
        m_writeTime = 0;
        m_writeSize = 0;
        return;
    }
    m_writeTime = time.Value();
    m_writeSize = size.Value();
    m_hasWriteTime = true;
}

bool Document::FileChangedOnDisk() const {
    if (!m_hasWriteTime || m_path.empty() || !Platform::Paths::Exists(m_path)) {
        return false;
    }
    auto time = Platform::Paths::LastWriteTimeCount(m_path);
    auto size = Platform::Paths::FileSize(m_path);
    if (!time.IsOk() || !size.IsOk()) {
        return false;
    }
    return time.Value() != m_writeTime || size.Value() != m_writeSize;
}

std::string Document::TextForDisk() const {
    if (m_lineEnding == "\n") {
        return m_text;
    }
    std::string out;
    out.reserve(m_text.size() * 2);
    for (char ch : m_text) {
        if (ch == '\n') {
            out += "\r\n";
        } else {
            out += ch;
        }
    }
    return out;
}

Core::Result<void> Document::LoadFromPath(const std::string& utf8Path) {
    auto text = Platform::Paths::ReadTextFile(utf8Path);
    if (!text.IsOk()) {
        return Core::Result<void>::Fail(text.GetError());
    }
    return LoadFromText(text.Value(), utf8Path);
}

Core::Result<void> Document::LoadFromText(const std::string& markdown, const std::string& utf8Path) {
    const ParseResult parsed = Parser::Parse(markdown);
    if (!parsed.utf8Valid || !parsed.completed) {
        return Core::Result<void>::Fail(Core::Error::Make(
            Core::ErrorCode::ParseFailure, "Script is not valid UTF-8", "剧本不是合法 UTF-8，已拒绝加载"));
    }
    m_lineEnding = ContainsCrlf(markdown) ? "\r\n" : "\n";
    m_text = NormalizeLf(markdown);
    if (m_text.size() >= 3 && static_cast<unsigned char>(m_text[0]) == 0xef &&
        static_cast<unsigned char>(m_text[1]) == 0xbb &&
        static_cast<unsigned char>(m_text[2]) == 0xbf) {
        m_text.erase(0, 3);
    }
    m_path = utf8Path;
    m_dirty = false;
    ApplyParse(parsed, true);
    RememberWriteTime();
    return Core::Result<void>::Ok();
}

Core::Result<void> Document::Save() {
    if (m_path.empty()) {
        return Core::Result<void>::Fail(Core::Error::Make(
            Core::ErrorCode::InvalidArgument, "Script has no path", "请先选择保存路径"));
    }
    return SaveToPath(m_path);
}

Core::Result<void> Document::SaveToPath(const std::string& utf8Path) {
    if (utf8Path.empty()) {
        return Core::Result<void>::Fail(Core::Error::Make(
            Core::ErrorCode::InvalidArgument, "Save path is empty", "保存路径不能为空"));
    }
    if (utf8Path == m_path && FileChangedOnDisk()) {
        return Core::Result<void>::Fail(Core::Error::Make(
            Core::ErrorCode::IoFailure, "Script changed on disk",
            "文件已被外部修改，请重新加载或另存"));
    }
    const std::string diskText = TextForDisk();
    auto written = Platform::Paths::WriteTextFile(utf8Path, diskText);
    if (!written.IsOk()) {
        return written;
    }
    m_path = utf8Path;
    m_dirty = false;
    RememberWriteTime();
    return Core::Result<void>::Ok();
}

void Document::SetText(const std::string& markdown) {
    const ParseResult parsed = Parser::Parse(markdown);
    if (!parsed.utf8Valid || !parsed.completed) {
        return;
    }
    m_text = NormalizeLf(markdown);
    m_dirty = true;
    ApplyParse(parsed, false);
}

void Document::InsertScene() {
    const std::string id = GenerateSceneId();
    if (!m_text.empty() && m_text.back() != '\n') {
        m_text += '\n';
    }
    m_text += "## [scene:" + id + "] 未命名\n\n";
    m_dirty = true;
    ApplyParse(Parser::Parse(m_text), true);
}

void Document::InsertShot(const std::string& afterShotId) {
    if (m_snapshot.scenes.empty()) {
        InsertScene();
    }
    const std::string id = GenerateShotId();
    const std::string block = "### [shot:" + id + "] 未命名\n\n";
    bool fallback = false;
    std::size_t insertAt = m_text.size();
    if (!afterShotId.empty()) {
        const Shot* after = FindShot(m_snapshot, afterShotId);
        if (after == nullptr) {
            fallback = true;
        } else {
            insertAt = OffsetOfLine(m_text, after->lineEnd + 1);
        }
    }
    if (insertAt >= m_text.size()) {
        if (!m_text.empty() && m_text.back() != '\n') {
            m_text += '\n';
        }
        m_text += block;
    } else {
        m_text.insert(insertAt, block);
    }
    m_dirty = true;
    ApplyParse(Parser::Parse(m_text), true);
    if (fallback) {
        Diagnostic diagnostic;
        diagnostic.severity = DiagnosticSeverity::Hint;
        diagnostic.line = 1;
        diagnostic.code = "script.unknown-after-shot";
        diagnostic.message = "未找到指定镜头，已追加到末尾";
        m_diagnostics.push_back(std::move(diagnostic));
    }
    SelectShot(id);
}

bool Document::RemoveShot(const std::string& shotId) {
    if (shotId.empty()) {
        return false;
    }
    const Shot* shot = FindShot(m_snapshot, shotId);
    if (shot == nullptr) {
        return false;
    }
    const std::size_t start = OffsetOfLine(m_text, shot->lineStart);
    const std::size_t end = OffsetOfLine(m_text, shot->lineEnd + 1);
    if (start > end) {
        return false;
    }
    m_text.erase(start, end - start);
    m_dirty = true;
    ApplyParse(Parser::Parse(m_text), true);
    if (m_selectedShotId.empty()) {
        SelectShot(FirstShotIdInSnapshot(m_snapshot));
    }
    return true;
}

bool Document::SetShotMeta(const std::string& shotId, const std::string& key, const std::string& value) {
    const std::string trimmedKey = TrimCopy(key);
    if (shotId.empty() || trimmedKey.empty()) {
        return false;
    }
    const Shot* shot = FindShot(m_snapshot, shotId);
    if (shot == nullptr) {
        return false;
    }
    const int firstLine = shot->headingLine + 1;
    const int lastLine = shot->lineEnd;
    std::vector<int> metaLines;
    std::vector<std::string> metaKeys;
    bool seenContent = false;
    for (int line = firstLine; line <= lastLine; ++line) {
        const std::size_t begin = OffsetOfLine(m_text, line);
        const std::size_t next = OffsetOfLine(m_text, line + 1);
        std::string text = m_text.substr(begin, next > begin ? next - begin : 0);
        while (!text.empty() && (text.back() == '\n' || text.back() == '\r')) {
            text.pop_back();
        }
        std::string parsedKey;
        std::string parsedValue;
        if (!seenContent && TrimCopy(text).empty()) {
            continue;
        }
        if (TryParseMetaLine(text, parsedKey, parsedValue)) {
            seenContent = true;
            metaLines.push_back(line);
            metaKeys.push_back(parsedKey);
            continue;
        }
        break;
    }
    int targetLine = 0;
    for (std::size_t i = 0; i < metaKeys.size(); ++i) {
        if (metaKeys[i] == trimmedKey) {
            targetLine = metaLines[i];
        }
    }
    const std::string trimmedValue = TrimCopy(value);
    if (targetLine > 0 && trimmedValue.empty()) {
        const std::size_t begin = OffsetOfLine(m_text, targetLine);
        const std::size_t end = OffsetOfLine(m_text, targetLine + 1);
        m_text.erase(begin, end - begin);
    } else if (targetLine > 0) {
        const std::size_t begin = OffsetOfLine(m_text, targetLine);
        const std::size_t end = OffsetOfLine(m_text, targetLine + 1);
        std::string ending = "\n";
        if (end > begin && m_text[end - 1] == '\n') {
            ending = (end > begin + 1 && m_text[end - 2] == '\r') ? "\r\n" : "\n";
        }
        m_text.replace(begin, end - begin, "> " + trimmedKey + ": " + trimmedValue + ending);
    } else if (trimmedValue.empty()) {
        return false;
    } else {
        const int insertLine = metaLines.empty() ? firstLine : metaLines.back() + 1;
        std::size_t insertAt = OffsetOfLine(m_text, insertLine);
        if (metaLines.empty()) {
            insertAt = OffsetOfLine(m_text, shot->headingLine + 1);
        }
        m_text.insert(insertAt, "> " + trimmedKey + ": " + trimmedValue + "\n");
    }
    m_dirty = true;
    ApplyParse(Parser::Parse(m_text), true);
    return true;
}

void Document::SelectShot(const std::string& shotId) {
    if (shotId.empty() || ShotExists(m_snapshot, shotId)) {
        m_selectedShotId = shotId;
    }
}

std::string Document::FirstShotId() const {
    if (!m_hasSnapshot) {
        return {};
    }
    return FirstShotIdInSnapshot(m_snapshot);
}

std::string Document::AdjacentShotId(int delta) const {
    if (!m_hasSnapshot || delta == 0) {
        return {};
    }
    std::vector<std::string> ids;
    for (const Scene& scene : m_snapshot.scenes) {
        for (const Shot& shot : scene.shots) {
            ids.push_back(shot.id);
        }
    }
    if (ids.empty()) {
        return {};
    }
    const int step = delta < 0 ? -1 : 1;
    int index = -1;
    if (!m_selectedShotId.empty()) {
        for (std::size_t i = 0; i < ids.size(); ++i) {
            if (ids[i] == m_selectedShotId) {
                index = static_cast<int>(i);
                break;
            }
        }
    }
    if (index < 0) {
        return step > 0 ? ids.front() : ids.back();
    }
    const int count = static_cast<int>(ids.size());
    index = (index + step + count) % count;
    return ids[static_cast<std::size_t>(index)];
}

void Document::Reset() {
    m_text.clear();
    m_path.clear();
    m_lineEnding = "\n";
    m_selectedShotId.clear();
    m_dirty = false;
    m_hasWriteTime = false;
    m_writeTime = 0;
    m_writeSize = 0;
    ApplyParse(Parser::Parse(""), true);
}

} // namespace DirectorDesk::Script
