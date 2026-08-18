#include "DirectorDesk/Export/ShotExport.h"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("Export resolutions match 1080p and 2K", "[export]") {
    const auto hd = DirectorDesk::Export::SizeFor(DirectorDesk::Export::ShotResolution::Hd1080);
    const auto uhd = DirectorDesk::Export::SizeFor(DirectorDesk::Export::ShotResolution::Uhd2k);
    REQUIRE(hd.width == 1920);
    REQUIRE(hd.height == 1080);
    REQUIRE(uhd.width == 2560);
    REQUIRE(uhd.height == 1440);
    const auto target = DirectorDesk::Export::MakeOffscreenTarget(
        DirectorDesk::Export::ShotResolution::Hd1080, true);
    REQUIRE(target.width == 1920);
    REQUIRE(target.transparentBackground);
    REQUIRE(target.kind == DirectorDesk::Renderer::RenderTargetKind::Offscreen);
}

TEST_CASE("Export file names include Chinese project and resolution", "[export]") {
    const std::string name = DirectorDesk::Export::DefaultShotFileName(
        "咖啡馆", "shot-cafe-001", DirectorDesk::Export::ShotResolution::Uhd2k, true);
    REQUIRE(name.find("咖啡馆") != std::string::npos);
    REQUIRE(name.find("shot-cafe-001") != std::string::npos);
    REQUIRE(name.find("2560x1440") != std::string::npos);
    REQUIRE(name.find("alpha") != std::string::npos);
}
