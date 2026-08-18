#include "DirectorDesk/Link/ShotLink.h"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("Shot links keep one camera per shot", "[link]") {
    DirectorDesk::Link::Table table;
    table.Set("shot-a", "cam-1");
    table.Set("shot-a", "cam-2");
    REQUIRE(table.All().size() == 1);
    REQUIRE(*table.CameraForShot("shot-a") == "cam-2");
    table.ClearShot("shot-a");
    REQUIRE(table.CameraForShot("shot-a") == nullptr);
}

TEST_CASE("Shot link table can be replaced", "[link]") {
    DirectorDesk::Link::Table table;
    table.Replace({{"shot-b", "cam-9"}});
    REQUIRE(table.All().size() == 1);
    REQUIRE(*table.CameraForShot("shot-b") == "cam-9");
}
