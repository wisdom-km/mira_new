// ParserTests: Implementation for the DirectorDesk Script module.
// This file owns project behavior only; keep platform and dependency boundaries explicit.
// Contract coverage: Markdown structure, diagnostics, IDs, and invalid-input retention.


#include "DirectorDesk/Script/Parser.h"

#include <catch2/catch_test_macros.hpp>

#include <string>

namespace {

const char* kSpecExample = R"(# 我的短片

## [scene:scene-cafe-day] 咖啡馆 - 日 - 内

场次说明写在这里。

### [shot:shot-cafe-001] 过肩

A 坐在窗边，看向街道。

### [shot:shot-cafe-002] 特写

A 的手指轻敲杯沿。

## [scene:scene-street-night] 街道 - 夜 - 外

### [shot:shot-street-001] 俯拍

雨中的空街。
)";

const DirectorDesk::Script::Diagnostic* FindCode(const DirectorDesk::Script::ParseResult& parsed,
                                                 const char* code) {
    for (const auto& diagnostic : parsed.diagnostics) {
        if (diagnostic.code == code) {
            return &diagnostic;
        }
    }
    return nullptr;
}

} // namespace

TEST_CASE("Spec example parses scenes and shots in order", "[script][parser]") {
    const auto parsed = DirectorDesk::Script::Parser::Parse(kSpecExample);
    REQUIRE(parsed.completed);
    REQUIRE(parsed.utf8Valid);
    REQUIRE(parsed.snapshot.documentTitle == "我的短片");
    REQUIRE(parsed.snapshot.scenes.size() == 2);
    REQUIRE(parsed.snapshot.scenes[0].id == "scene-cafe-day");
    REQUIRE(parsed.snapshot.scenes[0].title == "咖啡馆 - 日 - 内");
    REQUIRE(parsed.snapshot.scenes[0].shots.size() == 2);
    REQUIRE(parsed.snapshot.scenes[0].shots[0].id == "shot-cafe-001");
    REQUIRE(parsed.snapshot.scenes[0].shots[0].title == "过肩");
    REQUIRE(parsed.snapshot.scenes[0].shots[1].id == "shot-cafe-002");
    REQUIRE(parsed.snapshot.scenes[1].id == "scene-street-night");
    REQUIRE(parsed.snapshot.scenes[1].shots.size() == 1);
    REQUIRE(parsed.snapshot.scenes[1].shots[0].id == "shot-street-001");
    REQUIRE(parsed.snapshot.scenes[0].body.find("场次说明") != std::string::npos);
    REQUIRE(parsed.snapshot.scenes[0].shots[0].body.find("窗边") != std::string::npos);
}

TEST_CASE("Chinese titles and bodies survive parsing", "[script][parser]") {
    const auto parsed = DirectorDesk::Script::Parser::Parse(
        "## [scene:scene-cn] 中文场次\n\n说明：咖啡馆。\n\n### [shot:shot-cn] 中文镜头\n\n对白：你好。\n");
    REQUIRE(parsed.completed);
    REQUIRE(parsed.snapshot.scenes.front().title == "中文场次");
    REQUIRE(parsed.snapshot.scenes.front().shots.front().title == "中文镜头");
    REQUIRE(parsed.snapshot.scenes.front().shots.front().body.find("你好") != std::string::npos);
}

TEST_CASE("Duplicate and invalid IDs skip later nodes and keep first", "[script][parser]") {
    const char* text = "## [scene:scene-a] 一\n"
                       "### [shot:shot-a] 镜一\n"
                       "正文\n"
                       "## [scene:scene-a] 重复场次\n"
                       "### [shot:shot-b] 镜二\n"
                       "### [shot:BAD ID] 非法\n"
                       "### [shot:shot-a] 重复镜头\n"
                       "## [scene:SceneUpper] 非法场次\n"
                       "### [shot:shot-c] 镜三\n";
    const auto parsed = DirectorDesk::Script::Parser::Parse(text);
    REQUIRE(parsed.completed);
    REQUIRE(parsed.snapshot.scenes.size() == 1);
    REQUIRE(parsed.snapshot.scenes[0].shots.size() == 3);
    REQUIRE(parsed.snapshot.scenes[0].shots[0].id == "shot-a");
    REQUIRE(parsed.snapshot.scenes[0].shots[1].id == "shot-b");
    REQUIRE(parsed.snapshot.scenes[0].shots[2].id == "shot-c");
    REQUIRE(FindCode(parsed, "script.duplicate-id") != nullptr);
    REQUIRE(FindCode(parsed, "script.invalid-id") != nullptr);
}

TEST_CASE("Shot before scene is rejected with a line number", "[script][parser]") {
    const auto parsed = DirectorDesk::Script::Parser::Parse("### [shot:shot-early] 太早\n"
                                                            "## [scene:scene-a] 场\n"
                                                            "### [shot:shot-ok] 正常\n");
    REQUIRE(parsed.completed);
    REQUIRE(parsed.snapshot.scenes.size() == 1);
    REQUIRE(parsed.snapshot.scenes[0].shots.size() == 1);
    REQUIRE(parsed.snapshot.scenes[0].shots[0].id == "shot-ok");
    const auto* diagnostic = FindCode(parsed, "script.shot-before-scene");
    REQUIRE(diagnostic != nullptr);
    REQUIRE(diagnostic->line == 1);
    REQUIRE(diagnostic->severity == DirectorDesk::Script::DiagnosticSeverity::Error);
}

TEST_CASE("Fenced fake headings are not structure", "[script][parser]") {
    const char* text = "## [scene:scene-a] 场\n"
                       "```\n"
                       "### [shot:shot-fake] 伪镜头\n"
                       "```\n"
                       "### [shot:shot-real] 真镜头\n"
                       "正文\n";
    const auto parsed = DirectorDesk::Script::Parser::Parse(text);
    REQUIRE(parsed.completed);
    REQUIRE(parsed.snapshot.scenes.size() == 1);
    REQUIRE(parsed.snapshot.scenes[0].shots.size() == 1);
    REQUIRE(parsed.snapshot.scenes[0].shots[0].id == "shot-real");
}

TEST_CASE("CRLF LF and UTF-8 BOM parse the same structure", "[script][parser]") {
    const std::string lf = "## [scene:scene-a] 场\n### [shot:shot-a] 镜\n正文\n";
    std::string crlf;
    for (char ch : lf) {
        if (ch == '\n') {
            crlf += "\r\n";
        } else {
            crlf += ch;
        }
    }
    std::string bom = "\xEF\xBB\xBF" + lf;
    const auto parsedLf = DirectorDesk::Script::Parser::Parse(lf);
    const auto parsedCrlf = DirectorDesk::Script::Parser::Parse(crlf);
    const auto parsedBom = DirectorDesk::Script::Parser::Parse(bom);
    REQUIRE(parsedLf.snapshot.scenes.size() == 1);
    REQUIRE(parsedCrlf.snapshot.scenes.size() == 1);
    REQUIRE(parsedBom.snapshot.scenes.size() == 1);
    REQUIRE(parsedLf.snapshot.scenes[0].shots[0].id == parsedCrlf.snapshot.scenes[0].shots[0].id);
    REQUIRE(parsedBom.snapshot.scenes[0].shots[0].title == "镜");
}

TEST_CASE("Empty titles and empty shot bodies produce diagnostics", "[script][parser]") {
    const auto parsed = DirectorDesk::Script::Parser::Parse("## [scene:scene-a]\n### [shot:shot-a]\n");
    REQUIRE(parsed.completed);
    REQUIRE(parsed.snapshot.scenes[0].title == "未命名");
    REQUIRE(parsed.snapshot.scenes[0].shots[0].title == "未命名");
    REQUIRE(FindCode(parsed, "script.empty-title") != nullptr);
    REQUIRE(FindCode(parsed, "script.empty-shot-body") != nullptr);
}

TEST_CASE("Unmarked headings do not create nodes", "[script][parser]") {
    const auto parsed =
        DirectorDesk::Script::Parser::Parse("## 普通场次\n### 普通镜头\n## [scene:scene-a] 真场\n");
    REQUIRE(parsed.completed);
    REQUIRE(parsed.snapshot.scenes.size() == 1);
    REQUIRE(parsed.snapshot.scenes[0].id == "scene-a");
    REQUIRE(FindCode(parsed, "script.unmarked-heading") != nullptr);
}

TEST_CASE("Invalid UTF-8 does not complete a snapshot", "[script][parser]") {
    std::string bad = "## [scene:scene-a] 场\n";
    bad.push_back(static_cast<char>(0xff));
    const auto parsed = DirectorDesk::Script::Parser::Parse(bad);
    REQUIRE_FALSE(parsed.completed);
    REQUIRE_FALSE(parsed.utf8Valid);
    REQUIRE(parsed.snapshot.scenes.empty());
    REQUIRE(FindCode(parsed, "script.invalid-utf8") != nullptr);
}

TEST_CASE("H4 headings stay in shot body", "[script][parser]") {
    const auto parsed = DirectorDesk::Script::Parser::Parse(
        "## [scene:scene-a] 场\n### [shot:shot-a] 镜\n#### 细节\n继续\n");
    REQUIRE(parsed.completed);
    REQUIRE(parsed.snapshot.scenes[0].shots[0].body.find("#### 细节") != std::string::npos);
}
