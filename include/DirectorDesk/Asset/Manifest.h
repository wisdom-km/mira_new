#pragma once

#include "DirectorDesk/Core/Result.h"

#include <cstdint>
#include <string>
#include <vector>

namespace DirectorDesk::Asset {

struct LocalizedText {
    std::string zhCN;
    std::string en;
};

struct ManifestCategory {
    std::string id;
    LocalizedText name;
};

struct ManifestFile {
    std::string path;
    std::string url;
    std::string sha256;
    std::uint64_t size = 0;
};

struct ManifestLicense {
    std::string spdx;
    std::string name;
    std::string url;
    std::string attribution;
};

struct ManifestAuthor {
    std::string name;
    std::string url;
};

struct ManifestAsset {
    std::string id;
    std::string version;
    LocalizedText name;
    LocalizedText description;
    std::string category;
    std::vector<std::string> tags;
    std::string format;
    std::string entrypoint;
    std::string preview;
    std::vector<ManifestFile> files;
    ManifestLicense license;
    ManifestAuthor author;
    std::vector<std::string> diagnostics;
};

struct ManifestParseResult {
    bool accepted = false;
    std::string revision;
    std::string generatedAt;
    std::vector<ManifestCategory> categories;
    std::vector<ManifestAsset> assets;
    std::vector<std::string> diagnostics;
};

[[nodiscard]] std::string PickLocale(const LocalizedText& text);
[[nodiscard]] bool IsSafeRelativePath(const std::string& path);
[[nodiscard]] Core::Result<std::string> JoinOfficialUrl(const std::string& httpsBase,
                                                        const std::string& relative);
[[nodiscard]] ManifestParseResult ParseManifest(const std::string& jsonText);

} // namespace DirectorDesk::Asset
