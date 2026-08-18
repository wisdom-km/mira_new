#include "DirectorDesk/Asset/OfficialCatalog.h"
#include "DirectorDesk/Asset/Library.h"
#include "DirectorDesk/Core/Sha256.h"
#include "DirectorDesk/Platform/Paths.h"
#include "MockHttpClient.h"

#include <catch2/catch_test_macros.hpp>

#include <atomic>
#include <filesystem>
#include <string>

#ifdef DD_EXAMPLE_CUBE_GLB
#define DD_CUBE_PATH DD_EXAMPLE_CUBE_GLB
#else
#define DD_CUBE_PATH ""
#endif

namespace {

std::string TempRoot(const std::string& name) {
    const auto temp = DirectorDesk::Platform::Paths::TemporaryDirectory();
    REQUIRE(temp.IsOk());
    const std::string root = DirectorDesk::Platform::Paths::Join(
        DirectorDesk::Platform::Paths::Join(temp.Value(), "dd-official"), name);
    std::error_code ec;
    std::filesystem::remove_all(std::filesystem::u8path(root), ec);
    REQUIRE(DirectorDesk::Platform::Paths::CreateDirectories(root).IsOk());
    return root;
}

std::string ManifestJson(const std::string& sha, std::uint64_t size) {
    return std::string(R"({
      "schemaVersion": 1,
      "revision": "r1",
      "categories": [{"id": "prop", "name": {"zh-CN": "道具", "en": "Props"}}],
      "assets": [{
        "id": "basic-cube",
        "version": "1.0.0",
        "name": {"zh-CN": "测试立方体", "en": "Test Cube"},
        "description": {"zh-CN": "用于下载测试。", "en": "Download fixture."},
        "category": "prop",
        "tags": ["cube"],
        "format": "glb",
        "entrypoint": "model/cube.glb",
        "files": [{
          "path": "model/cube.glb",
          "url": "assets/basic-cube/1.0.0/cube.glb",
          "sha256": ")") +
           sha + R"(",
          "size": )" +
           std::to_string(size) + R"(
        }],
        "license": {"spdx": "CC0-1.0", "name": "CC0", "attribution": ""},
        "author": {"name": "DirectorDesk Community"}
      }]
    })";
}

} // namespace

TEST_CASE("Failed refresh keeps the last valid cache", "[asset][official]") {
    const std::string root = TempRoot("cache-zh");
    DirectorDesk::Asset::OfficialEndpoints endpoints;
    endpoints.manifestUrl = "https://official.example/manifest.json";
    endpoints.assetBaseUrl = "https://official.example/files/";
    DirectorDesk::Asset::OfficialCatalog catalog(root, endpoints);
    DirectorDesk::Tests::MockHttpClient http;
    const std::string json = ManifestJson(
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa", 1);
    http.routes[endpoints.manifestUrl] = {200, std::vector<std::uint8_t>(json.begin(), json.end())};
    REQUIRE(catalog.Refresh(http, nullptr).IsOk());
    REQUIRE(catalog.FindAsset("basic-cube") != nullptr);

    http.routes[endpoints.manifestUrl] = {500, {}};
    REQUIRE_FALSE(catalog.Refresh(http, nullptr).IsOk());
    REQUIRE(catalog.UsedCachedManifest());
    REQUIRE(catalog.FindAsset("basic-cube") != nullptr);
}

TEST_CASE("Download verifies hash and indexes a ready official asset", "[asset][official]") {
    REQUIRE(std::string(DD_CUBE_PATH).size() > 0);
    auto bytes = DirectorDesk::Platform::Paths::ReadBinaryFile(DD_CUBE_PATH);
    REQUIRE(bytes.IsOk());
    const std::string sha = DirectorDesk::Core::Sha256Hex(bytes.Value());
    const std::string root = TempRoot("download-ok");
    DirectorDesk::Asset::OfficialEndpoints endpoints;
    endpoints.manifestUrl = "https://official.example/manifest.json";
    endpoints.assetBaseUrl = "https://official.example/files/";
    DirectorDesk::Asset::OfficialCatalog catalog(root, endpoints);
    DirectorDesk::Tests::MockHttpClient http;
    const std::string json = ManifestJson(sha, bytes.Value().size());
    http.routes[endpoints.manifestUrl] = {200, std::vector<std::uint8_t>(json.begin(), json.end())};
    http.routes["https://official.example/files/assets/basic-cube/1.0.0/cube.glb"] = {
        200, bytes.Value()};
    REQUIRE(catalog.Refresh(http, nullptr).IsOk());
    REQUIRE(catalog.Download("basic-cube", http, nullptr, {}).IsOk());
    REQUIRE(catalog.State("basic-cube")->status ==
            DirectorDesk::Asset::OfficialDownloadStatus::Ready);
    REQUIRE(DirectorDesk::Platform::Paths::Exists(catalog.State("basic-cube")->entrypointPath));

    DirectorDesk::Asset::Library library;
    REQUIRE(library.Open(DirectorDesk::Platform::Paths::Join(root, "library")).IsOk());
    DirectorDesk::Asset::LibraryAsset asset;
    asset.id = "basic-cube";
    asset.name = "测试立方体";
    asset.sourcePath = catalog.State("basic-cube")->entrypointPath;
    asset.format = "glb";
    asset.origin = DirectorDesk::Asset::AssetOrigin::OnlineCache;
    REQUIRE(library.Upsert(asset).IsOk());
    REQUIRE(library.Find("basic-cube") != nullptr);
    REQUIRE(library.Query("", "online").size() == 1);
}

TEST_CASE("Hash mismatch never enters the official cache", "[asset][official]") {
    auto bytes = DirectorDesk::Platform::Paths::ReadBinaryFile(DD_CUBE_PATH);
    REQUIRE(bytes.IsOk());
    std::vector<std::uint8_t> bad = bytes.Value();
    if (!bad.empty()) {
        bad[0] ^= 0xff;
    }
    const std::string sha = DirectorDesk::Core::Sha256Hex(bytes.Value());
    const std::string root = TempRoot("hash-bad");
    DirectorDesk::Asset::OfficialEndpoints endpoints;
    endpoints.manifestUrl = "https://official.example/manifest.json";
    endpoints.assetBaseUrl = "https://official.example/files/";
    DirectorDesk::Asset::OfficialCatalog catalog(root, endpoints);
    DirectorDesk::Tests::MockHttpClient http;
    const std::string json = ManifestJson(sha, bytes.Value().size());
    http.routes[endpoints.manifestUrl] = {200, std::vector<std::uint8_t>(json.begin(), json.end())};
    http.routes["https://official.example/files/assets/basic-cube/1.0.0/cube.glb"] = {200, bad};
    REQUIRE(catalog.Refresh(http, nullptr).IsOk());
    REQUIRE_FALSE(catalog.Download("basic-cube", http, nullptr, {}).IsOk());
    REQUIRE(catalog.State("basic-cube")->failure ==
            DirectorDesk::Asset::OfficialFailureKind::HashMismatch);
    REQUIRE_FALSE(DirectorDesk::Platform::Paths::Exists(
        DirectorDesk::Platform::Paths::Join(root, "official/basic-cube/1.0.0/model/cube.glb")));
}

TEST_CASE("Size mismatch and user cancel are distinct failures", "[asset][official]") {
    auto bytes = DirectorDesk::Platform::Paths::ReadBinaryFile(DD_CUBE_PATH);
    REQUIRE(bytes.IsOk());
    const std::string sha = DirectorDesk::Core::Sha256Hex(bytes.Value());
    const std::string root = TempRoot("size-cancel");
    DirectorDesk::Asset::OfficialEndpoints endpoints;
    endpoints.manifestUrl = "https://official.example/manifest.json";
    endpoints.assetBaseUrl = "https://official.example/files/";
    DirectorDesk::Asset::OfficialCatalog catalog(root, endpoints);
    DirectorDesk::Tests::MockHttpClient http;
    const std::string json = ManifestJson(sha, bytes.Value().size() + 8);
    http.routes[endpoints.manifestUrl] = {200, std::vector<std::uint8_t>(json.begin(), json.end())};
    http.routes["https://official.example/files/assets/basic-cube/1.0.0/cube.glb"] = {
        200, bytes.Value()};
    REQUIRE(catalog.Refresh(http, nullptr).IsOk());
    REQUIRE_FALSE(catalog.Download("basic-cube", http, nullptr, {}).IsOk());
    REQUIRE(catalog.State("basic-cube")->failure ==
            DirectorDesk::Asset::OfficialFailureKind::SizeMismatch);

    catalog.SetAvailableBytesOverride(1);
    REQUIRE_FALSE(catalog.Download("basic-cube", http, nullptr, {}).IsOk());
    REQUIRE(catalog.State("basic-cube")->failure ==
            DirectorDesk::Asset::OfficialFailureKind::DiskFull);

    catalog.ClearAvailableBytesOverride();
    std::atomic<bool> cancel{true};
    REQUIRE_FALSE(catalog.Download("basic-cube", http, &cancel, {}).IsOk());
    REQUIRE(catalog.State("basic-cube")->status ==
            DirectorDesk::Asset::OfficialDownloadStatus::Cancelled);
}

TEST_CASE("Unconfigured catalog can still load a cached manifest", "[asset][official]") {
    const std::string root = TempRoot("unconfigured");
    DirectorDesk::Asset::OfficialEndpoints live;
    live.manifestUrl = "https://official.example/manifest.json";
    live.assetBaseUrl = "https://official.example/files/";
    DirectorDesk::Asset::OfficialCatalog writer(root, live);
    DirectorDesk::Tests::MockHttpClient http;
    const std::string json = ManifestJson(
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa", 1);
    http.routes[live.manifestUrl] = {200, std::vector<std::uint8_t>(json.begin(), json.end())};
    REQUIRE(writer.Refresh(http, nullptr).IsOk());

    DirectorDesk::Asset::OfficialCatalog cached(root, {});
    REQUIRE_FALSE(cached.IsConfigured());
    REQUIRE(cached.LoadCache().IsOk());
    REQUIRE(cached.FindAsset("basic-cube") != nullptr);
}
