// CommandTests: UI-PRO command variants compile and hold the documented fields.

#include "DirectorDesk/Core/Command.h"

#include <catch2/catch_test_macros.hpp>
#include <variant>

TEST_CASE("UI-PRO commands join the Command variant") {
    using DirectorDesk::Core::Command;
    Command mode = DirectorDesk::Core::SetWorkspaceModeCommand{"review"};
    Command reset = DirectorDesk::Core::ResetLayoutCommand{};
    Command bind = DirectorDesk::Core::BindShotToNewCameraCommand{"shot-1"};
    Command resolution = DirectorDesk::Core::SelectExportResolutionCommand{"2k"};

    REQUIRE(std::holds_alternative<DirectorDesk::Core::SetWorkspaceModeCommand>(mode));
    REQUIRE(std::get<DirectorDesk::Core::SetWorkspaceModeCommand>(mode).modeId == "review");
    REQUIRE(std::holds_alternative<DirectorDesk::Core::ResetLayoutCommand>(reset));
    REQUIRE(std::get<DirectorDesk::Core::BindShotToNewCameraCommand>(bind).shotId == "shot-1");
    REQUIRE(std::get<DirectorDesk::Core::SelectExportResolutionCommand>(resolution).resolutionId ==
            "2k");
}

TEST_CASE("DeleteShotCommand and RemoveLibraryAssetCommand join the variant") {
    using DirectorDesk::Core::Command;
    Command del = DirectorDesk::Core::DeleteShotCommand{"shot-a"};
    Command remove = DirectorDesk::Core::RemoveLibraryAssetCommand{"local-1"};
    REQUIRE(std::get<DirectorDesk::Core::DeleteShotCommand>(del).shotId == "shot-a");
    REQUIRE(std::get<DirectorDesk::Core::RemoveLibraryAssetCommand>(remove).assetId == "local-1");
}

TEST_CASE("SelectAdjacentShotCommand joins the variant") {
    using DirectorDesk::Core::Command;
    Command next = DirectorDesk::Core::SelectAdjacentShotCommand{};
    Command prev = DirectorDesk::Core::SelectAdjacentShotCommand{-1};
    REQUIRE(std::get<DirectorDesk::Core::SelectAdjacentShotCommand>(next).delta == 1);
    REQUIRE(std::get<DirectorDesk::Core::SelectAdjacentShotCommand>(prev).delta == -1);
}

TEST_CASE("RevealPathCommand joins the variant") {
    using DirectorDesk::Core::Command;
    Command open = DirectorDesk::Core::RevealPathCommand{"D:/shot.png", false};
    Command folder = DirectorDesk::Core::RevealPathCommand{"D:/shot.png", true};
    REQUIRE(std::get<DirectorDesk::Core::RevealPathCommand>(open).utf8Path == "D:/shot.png");
    REQUIRE_FALSE(std::get<DirectorDesk::Core::RevealPathCommand>(open).folder);
    REQUIRE(std::get<DirectorDesk::Core::RevealPathCommand>(folder).folder);
}

TEST_CASE("SetWorkspaceModeCommand defaults to shoot") {
    DirectorDesk::Core::SetWorkspaceModeCommand command;
    REQUIRE(command.modeId == "shoot");
}

TEST_CASE("ExportCurrentShotCommand default resolution is empty") {
    DirectorDesk::Core::ExportCurrentShotCommand command;
    REQUIRE(command.resolutionId.empty());
}
