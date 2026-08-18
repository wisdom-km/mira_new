#pragma once

#include "DirectorDesk/Core/Result.h"
#include "DirectorDesk/Script/Types.h"

#include <cstdint>
#include <string>

namespace DirectorDesk::Script {

class Document {
public:
    Document();

    Core::Result<void> LoadFromPath(const std::string& utf8Path);
    Core::Result<void> LoadFromText(const std::string& markdown, const std::string& utf8Path = {});
    Core::Result<void> Save();
    Core::Result<void> SaveToPath(const std::string& utf8Path);

    void SetText(const std::string& markdown);
    void InsertScene();
    void InsertShot();
    void SelectShot(const std::string& shotId);

    [[nodiscard]] const std::string& Text() const {
        return m_text;
    }
    [[nodiscard]] const std::string& Path() const {
        return m_path;
    }
    [[nodiscard]] bool IsDirty() const {
        return m_dirty;
    }
    [[nodiscard]] bool HasPublishedSnapshot() const {
        return m_hasSnapshot;
    }
    [[nodiscard]] const Snapshot& PublishedSnapshot() const {
        return m_snapshot;
    }
    [[nodiscard]] const std::vector<Diagnostic>& Diagnostics() const {
        return m_diagnostics;
    }
    [[nodiscard]] const std::string& SelectedShotId() const {
        return m_selectedShotId;
    }
    [[nodiscard]] std::uint64_t ExternalRevision() const {
        return m_externalRevision;
    }

private:
    void ApplyParse(const ParseResult& parsed, bool incrementRevision);
    void RememberWriteTime();
    [[nodiscard]] bool FileChangedOnDisk() const;
    [[nodiscard]] std::string TextForDisk() const;

    std::string m_text;
    std::string m_path;
    std::string m_lineEnding = "\n";
    Snapshot m_snapshot;
    std::vector<Diagnostic> m_diagnostics;
    std::string m_selectedShotId;
    bool m_hasSnapshot = false;
    bool m_dirty = false;
    bool m_hasWriteTime = false;
    std::uint64_t m_writeTime = 0;
    std::uint64_t m_externalRevision = 1;
};

} // namespace DirectorDesk::Script
