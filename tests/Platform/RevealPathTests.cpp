// RevealPathTests: blocked paths never call the system file manager.

#include "DirectorDesk/Platform/Paths.h"
#include "DirectorDesk/Platform/RevealPath.h"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("RevealPath rejects empty remote and relative paths", "[platform][reveal]") {
    using DirectorDesk::Platform::RevealPath;
    using DirectorDesk::Platform::RevealPathIsBlocked;

    REQUIRE(RevealPathIsBlocked(""));
    REQUIRE(RevealPathIsBlocked("https://example.com/shot.png"));
    REQUIRE(RevealPathIsBlocked("file://C:/shot.png"));
    REQUIRE(RevealPathIsBlocked("shot.png"));
    REQUIRE(RevealPathIsBlocked("exports/shot.png"));

    auto empty = RevealPath("", false);
    REQUIRE_FALSE(empty.IsOk());
    REQUIRE(empty.GetError().userMessage == "无法打开路径");

    auto remote = RevealPath("https://example.com/shot.png", true);
    REQUIRE_FALSE(remote.IsOk());
    REQUIRE(remote.GetError().userMessage == "无法打开路径");

    auto relative = RevealPath("exports/shot.png", false);
    REQUIRE_FALSE(relative.IsOk());
    REQUIRE(relative.GetError().code == DirectorDesk::Core::ErrorCode::InvalidArgument);
}

TEST_CASE("RevealPath allows an absolute local path", "[platform][reveal]") {
    auto temp = DirectorDesk::Platform::Paths::TemporaryDirectory();
    REQUIRE(temp.IsOk());
    REQUIRE_FALSE(DirectorDesk::Platform::RevealPathIsBlocked(temp.Value()));
}
