#include "DirectorDesk/Platform/Paths.h"

#include <catch2/catch_test_macros.hpp>

#include <string>

TEST_CASE("Paths can create and detect a Chinese user-data directory", "[platform][paths]") {
    auto temp = DirectorDesk::Platform::Paths::TemporaryDirectory();
    REQUIRE(temp.IsOk());

    const std::string root = DirectorDesk::Platform::Paths::Join(
        temp.Value(), "导演台用户数据 DirectorDesk");
    const std::string nested = DirectorDesk::Platform::Paths::Join(root, "logs");

    auto created = DirectorDesk::Platform::Paths::CreateDirectories(nested);
    REQUIRE(created.IsOk());
    REQUIRE(DirectorDesk::Platform::Paths::Exists(nested));
    REQUIRE(DirectorDesk::Platform::Paths::IsDirectory(nested));
    REQUIRE(DirectorDesk::Platform::Paths::FileName(nested) == "logs");
}

TEST_CASE("UserDataDirectory is rooted at DirectorDesk", "[platform][paths]") {
    auto userData = DirectorDesk::Platform::Paths::UserDataDirectory();
    REQUIRE(userData.IsOk());
    REQUIRE(DirectorDesk::Platform::Paths::FileName(userData.Value()) == "DirectorDesk");

    auto logs = DirectorDesk::Platform::Paths::LogDirectory();
    REQUIRE(logs.IsOk());
    REQUIRE(DirectorDesk::Platform::Paths::FileName(logs.Value()) == "logs");
}

TEST_CASE("CreateDirectories rejects an empty path", "[platform][paths]") {
    auto result = DirectorDesk::Platform::Paths::CreateDirectories("");
    REQUIRE_FALSE(result.IsOk());
    REQUIRE(result.GetError().code == DirectorDesk::Core::ErrorCode::InvalidArgument);
}
