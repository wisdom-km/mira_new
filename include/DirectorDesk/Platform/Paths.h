#pragma once

#include "DirectorDesk/Core/Result.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace DirectorDesk::Platform {

class Paths {
public:
    static std::string Join(const std::string& leftUtf8, const std::string& rightUtf8);
    static Core::Result<void> CreateDirectories(const std::string& utf8Path);
    [[nodiscard]] static bool Exists(const std::string& utf8Path);
    [[nodiscard]] static bool IsDirectory(const std::string& utf8Path);

    // Platform user-data root plus "/DirectorDesk".
    static Core::Result<std::string> UserDataDirectory();
    static Core::Result<std::string> LogDirectory();
    static Core::Result<std::string> LibraryDirectory();
    static Core::Result<std::string> TemporaryDirectory();
    static Core::Result<std::string> WeaklyCanonical(const std::string& utf8Path);
    static Core::Result<std::uint64_t> FileSize(const std::string& utf8Path);
    [[nodiscard]] static std::string StableKey(const std::string& utf8Path);

    static std::string FileName(const std::string& utf8Path);
    static std::string Stem(const std::string& utf8Path);
    static std::string Parent(const std::string& utf8Path);
    static std::string ExtensionLower(const std::string& utf8Path);
    static Core::Result<std::string> ReadTextFile(const std::string& utf8Path);
    static Core::Result<void> WriteTextFile(const std::string& utf8Path, const std::string& utf8Text);
    static Core::Result<std::uint64_t> LastWriteTimeCount(const std::string& utf8Path);
    static Core::Result<std::string> ExecutableDirectory();
    static Core::Result<std::string> UiFontFile();
    static Core::Result<std::vector<std::uint8_t>> ReadBinaryFile(const std::string& utf8Path);
    static Core::Result<void> WriteBinaryFile(const std::string& utf8Path, const std::uint8_t* data,
                                              std::size_t size);
    static Core::Result<void> RemoveFile(const std::string& utf8Path);
    static Core::Result<void> AtomicReplace(const std::string& fromUtf8, const std::string& toUtf8);
    [[nodiscard]] static bool IsAbsolute(const std::string& utf8Path);
    [[nodiscard]] static bool IsWithin(const std::string& rootUtf8, const std::string& pathUtf8);
    static Core::Result<std::string> RelativeTo(const std::string& rootUtf8,
                                                const std::string& pathUtf8);
    [[nodiscard]] static std::string NormalizeSlashes(const std::string& utf8Path);
};

} // namespace DirectorDesk::Platform
