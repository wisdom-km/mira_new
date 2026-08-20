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

TEST_CASE("SetWorkspaceModeCommand defaults to shoot") {
    DirectorDesk::Core::SetWorkspaceModeCommand command;
    REQUIRE(command.modeId == "shoot");
}
