#include "DirectorDesk/Asset/Manifest.h"
#include "DirectorDesk/Core/Sha256.h"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("Locale prefers Chinese and falls back to English", "[asset][manifest]") {
    DirectorDesk::Asset::LocalizedText text;
    text.en = "Chair";
    REQUIRE(DirectorDesk::Asset::PickLocale(text) == "Chair");
    text.zhCN = "椅子";
    REQUIRE(DirectorDesk::Asset::PickLocale(text) == "椅子");
}

TEST_CASE("Official URL join rejects traversal and host jumps", "[asset][manifest]") {
    const std::string base = "https://official.example/dd/";
    REQUIRE_FALSE(DirectorDesk::Asset::JoinOfficialUrl(base, "../escape").IsOk());
    REQUIRE_FALSE(DirectorDesk::Asset::JoinOfficialUrl(base, "/abs").IsOk());
    REQUIRE_FALSE(DirectorDesk::Asset::JoinOfficialUrl(base, "https://evil.example/x").IsOk());
    REQUIRE_FALSE(DirectorDesk::Asset::JoinOfficialUrl("http://official.example/dd/", "a.glb").IsOk());
    auto ok = DirectorDesk::Asset::JoinOfficialUrl(base, "assets/chair.glb");
    REQUIRE(ok.IsOk());
    REQUIRE(ok.Value() == "https://official.example/dd/assets/chair.glb");
}

TEST_CASE("Valid GLB manifest keeps unknown fields", "[asset][manifest]") {
    const char* json =
        R"({
          "schemaVersion": 1,
          "revision": "2026-08-19.1",
          "generatedAt": "2026-08-19T00:00:00Z",
          "extraRoot": true,
          "categories": [{"id": "prop", "name": {"zh-CN": "道具", "en": "Props"}}],
          "assets": [{
            "id": "basic-chair",
            "version": "1.0.0",
            "name": {"zh-CN": "基础椅子", "en": "Basic Chair"},
            "description": {"zh-CN": "一把椅子。", "en": "A chair."},
            "category": "prop",
            "tags": ["chair", "Chair", "furniture"],
            "format": "glb",
            "entrypoint": "model/chair.glb",
            "preview": "preview/cover.png",
            "files": [
              {"path": "model/chair.glb", "url": "assets/basic-chair/1.0.0/chair.glb",
               "sha256": "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa", "size": 12},
              {"path": "preview/cover.png", "url": "assets/basic-chair/1.0.0/cover.png",
               "sha256": "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb", "size": 4}
            ],
            "license": {"spdx": "CC0-1.0", "name": "CC0", "url": "https://creativecommons.org", "attribution": ""},
            "author": {"name": "DirectorDesk Community", "url": "https://example.invalid"},
            "future": 1
          }]
        })";
    const auto parsed = DirectorDesk::Asset::ParseManifest(json);
    REQUIRE(parsed.accepted);
    REQUIRE(parsed.assets.size() == 1);
    REQUIRE(parsed.assets[0].tags.size() == 2);
    REQUIRE(DirectorDesk::Asset::PickLocale(parsed.assets[0].name) == "基础椅子");
}

TEST_CASE("OBJ assets and invalid entries are filtered", "[asset][manifest]") {
    const char* json =
        R"({
          "schemaVersion": 1,
          "revision": "r1",
          "categories": [{"id": "prop", "name": {"en": "Props"}}],
          "assets": [
            {
              "id": "table-obj",
              "version": "1.2.3",
              "name": {"en": "Table"},
              "category": "prop",
              "format": "obj",
              "entrypoint": "model/table.obj",
              "preview": "preview/cover.png",
              "files": [
                {"path": "model/table.obj", "url": "assets/table/table.obj",
                 "sha256": "cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc", "size": 2},
                {"path": "model/table.mtl", "url": "assets/table/table.mtl",
                 "sha256": "dddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddd", "size": 2},
                {"path": "preview/cover.png", "url": "assets/table/cover.png",
                 "sha256": "eeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee", "size": 2}
              ],
              "license": {"spdx": "CC-BY-4.0", "name": "CC BY", "attribution": "Name"},
              "author": {"name": "Author"}
            },
            {
              "id": "bad id",
              "version": "1.0.0",
              "name": {"en": "Bad"},
              "category": "prop",
              "format": "glb",
              "entrypoint": "a.glb",
              "files": [{"path": "a.glb", "url": "a.glb",
                         "sha256": "ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff", "size": 1}],
              "license": {"spdx": "CC0-1.0"},
              "author": {"name": "A"}
            },
            {
              "id": "nolicense",
              "version": "not-semver",
              "name": {"en": "X"},
              "category": "missing",
              "format": "fbx",
              "entrypoint": "a.glb",
              "files": [],
              "license": {"spdx": "MIT"},
              "author": {"name": ""}
            }
          ]
        })";
    const auto parsed = DirectorDesk::Asset::ParseManifest(json);
    REQUIRE(parsed.accepted);
    REQUIRE(parsed.assets.size() == 1);
    REQUIRE(parsed.assets[0].id == "table-obj");
}

TEST_CASE("High schema version and missing root fields reject the manifest", "[asset][manifest]") {
    REQUIRE_FALSE(DirectorDesk::Asset::ParseManifest(
                      R"({"schemaVersion":2,"revision":"r","categories":[],"assets":[]})")
                      .accepted);
    REQUIRE_FALSE(DirectorDesk::Asset::ParseManifest(R"({"schemaVersion":1,"assets":[]})").accepted);
}

TEST_CASE("Duplicate asset versions are rejected", "[asset][manifest]") {
    const char* json =
        R"({
          "schemaVersion": 1,
          "revision": "r1",
          "categories": [{"id": "prop", "name": {"en": "P"}}],
          "assets": [
            {
              "id": "dup", "version": "1.0.0", "name": {"en": "A"}, "category": "prop",
              "format": "glb", "entrypoint": "a.glb",
              "files": [{"path": "a.glb", "url": "a.glb",
                         "sha256": "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa", "size": 1}],
              "license": {"spdx": "CC0-1.0"}, "author": {"name": "A"}
            },
            {
              "id": "dup", "version": "1.0.0", "name": {"en": "B"}, "category": "prop",
              "format": "glb", "entrypoint": "a.glb",
              "files": [{"path": "a.glb", "url": "a.glb",
                         "sha256": "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa", "size": 1}],
              "license": {"spdx": "CC0-1.0"}, "author": {"name": "A"}
            }
          ]
        })";
    const auto parsed = DirectorDesk::Asset::ParseManifest(json);
    REQUIRE(parsed.accepted);
    REQUIRE(parsed.assets.size() == 1);
}

TEST_CASE("Published official manifest URLs stay on the designated host", "[asset][manifest]") {
    const std::string base = "https://raw.githubusercontent.com/wisdom-km/obj-3d-models/main/";
    auto url = DirectorDesk::Asset::JoinOfficialUrl(base, "models/basic-cube/1.0.0/cube.obj");
    REQUIRE(url.IsOk());
    REQUIRE(url.Value() ==
            "https://raw.githubusercontent.com/wisdom-km/obj-3d-models/main/models/basic-cube/1.0.0/cube.obj");
}

TEST_CASE("Sha256 helper matches a known empty digest", "[core][sha256]") {
    REQUIRE(DirectorDesk::Core::Sha256Hex(std::vector<std::uint8_t>{}) ==
            "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
}
