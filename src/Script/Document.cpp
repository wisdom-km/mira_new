#include "DirectorDesk/Script/Document.h"

#include "DirectorDesk/Core/Error.h"
#include "DirectorDesk/Platform/Paths.h"
#include "DirectorDesk/Script/Ids.h"
#include "DirectorDesk/Script/Parser.h"

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

bool ShotExists(const Snapshot& snapshot, const std::string& shotId) {
    for (const Scene& scene : snapshot.scenes) {
        for (const Shot& shot : scene.shots) {
            if (shot.id == shotId) {
                return true;
            }
        }
    }
    return false;
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
        return;
    }
    auto time = Platform::Paths::LastWriteTimeCount(m_path);
    if (!time.IsOk()) {
        m_hasWriteTime = false;
        m_writeTime = 0;
        return;
    }
    m_writeTime = time.Value();
    m_hasWriteTime = true;
}

bool Document::FileChangedOnDisk() const {
    if (!m_hasWriteTime || m_path.empty() || !Platform::Paths::Exists(m_path)) {
        return false;
    }
    auto time = Platform::Paths::LastWriteTimeCount(m_path);
    return time.IsOk() && time.Value() != m_writeTime;
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

void Document::InsertShot() {
    if (m_snapshot.scenes.empty()) {
        InsertScene();
    }
    const std::string id = GenerateShotId();
    if (!m_text.empty() && m_text.back() != '\n') {
        m_text += '\n';
    }
    m_text += "### [shot:" + id + "] 未命名\n\n";
    m_dirty = true;
    ApplyParse(Parser::Parse(m_text), true);
    SelectShot(id);
}

void Document::SelectShot(const std::string& shotId) {
    if (shotId.empty() || ShotExists(m_snapshot, shotId)) {
        m_selectedShotId = shotId;
    }
}

void Document::Reset() {
    m_text.clear();
    m_path.clear();
    m_lineEnding = "\n";
    m_selectedShotId.clear();
    m_dirty = false;
    m_hasWriteTime = false;
    m_writeTime = 0;
    ApplyParse(Parser::Parse(""), true);
}

} // namespace DirectorDesk::Script
