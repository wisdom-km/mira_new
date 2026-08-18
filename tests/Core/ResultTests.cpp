// ResultTests: Implementation for the DirectorDesk Core module.
// This file owns project behavior only; keep platform and dependency boundaries explicit.
// Contract coverage: Result carries either a value or an Error without ambiguous states.


#include "DirectorDesk/Core/Result.h"

#include <catch2/catch_test_macros.hpp>
#include <string>

TEST_CASE("Result carries a value on success", "[core]") {
    const auto result = DirectorDesk::Core::Result<int>::Ok(7);
    REQUIRE(result.IsOk());
    REQUIRE(result.Value() == 7);
}

TEST_CASE("Result carries an error on failure", "[core]") {
    const auto result = DirectorDesk::Core::Result<std::string>::Fail(
        DirectorDesk::Core::Error::Make(
            DirectorDesk::Core::ErrorCode::NotFound,
            "missing",
            "未找到文件"));
    REQUIRE_FALSE(result.IsOk());
    REQUIRE(result.GetError().code == DirectorDesk::Core::ErrorCode::NotFound);
    REQUIRE(result.GetError().userMessage == "未找到文件");
}

TEST_CASE("void Result represents success without a value", "[core]") {
    const auto result = DirectorDesk::Core::Result<void>::Ok();
    REQUIRE(result.IsOk());
}
