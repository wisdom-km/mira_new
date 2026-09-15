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

TEST_CASE("InsertShotCommand afterShotId defaults to empty") {
    DirectorDesk::Core::InsertShotCommand command;
    REQUIRE(command.afterShotId.empty());
}

TEST_CASE("ExportCurrentShotCommand default resolution is empty") {
    DirectorDesk::Core::ExportCurrentShotCommand command;
    REQUIRE(command.resolutionId.empty());
}

TEST_CASE("DeleteNode DuplicateNode and SetNodeVisible join the variant") {
    using DirectorDesk::Core::Command;
    Command del = DirectorDesk::Core::DeleteNodeCommand{"node-1"};
    Command dup = DirectorDesk::Core::DuplicateNodeCommand{"node-1"};
    Command hide = DirectorDesk::Core::SetNodeVisibleCommand{"node-1", false};
    REQUIRE(std::get<DirectorDesk::Core::DeleteNodeCommand>(del).nodeId == "node-1");
    REQUIRE(std::get<DirectorDesk::Core::DuplicateNodeCommand>(dup).nodeId == "node-1");
    REQUIRE_FALSE(std::get<DirectorDesk::Core::SetNodeVisibleCommand>(hide).visible);
}

TEST_CASE("SetShotMeta ExportShotPackage and ImportStoryboard join the variant") {
    using DirectorDesk::Core::Command;
    Command meta = DirectorDesk::Core::SetShotMetaCommand{"shot-a", "景别", "中景"};
    Command pack = DirectorDesk::Core::ExportShotPackageCommand{"shot-a", "2k"};
    Command importDlg = DirectorDesk::Core::ImportStoryboardCommand{"replace"};
    Command importPath = DirectorDesk::Core::ImportStoryboardFromPathCommand{"a.json", "append"};
    REQUIRE(std::get<DirectorDesk::Core::SetShotMetaCommand>(meta).key == "景别");
    REQUIRE(std::get<DirectorDesk::Core::ExportShotPackageCommand>(pack).resolutionId == "2k");
    REQUIRE(std::get<DirectorDesk::Core::ImportStoryboardCommand>(importDlg).mode == "replace");
    REQUIRE(std::get<DirectorDesk::Core::ImportStoryboardFromPathCommand>(importPath).mode ==
            "append");
    DirectorDesk::Core::ImportStoryboardCommand defaultImport;
    REQUIRE(defaultImport.mode == "append");
}

TEST_CASE("AI commands join the variant", "[core][command][fnd53]") {
    using DirectorDesk::Core::Command;
    Command image = DirectorDesk::Core::GenerateShotImageCommand{"shot-a"};
    Command video = DirectorDesk::Core::GenerateShotVideoCommand{"shot-a"};
    Command cancel = DirectorDesk::Core::CancelAiJobCommand{};
    Command settings = DirectorDesk::Core::SetAiSettingsCommand{"mock", "https://api.example.com",
                                                                "sk-test", "gpt-image-1", "sora-2"};
    Command run = DirectorDesk::Core::RunSkillCommand{"skill-1"};
    REQUIRE(std::get<DirectorDesk::Core::GenerateShotImageCommand>(image).shotId == "shot-a");
    REQUIRE(std::get<DirectorDesk::Core::GenerateShotVideoCommand>(video).shotId == "shot-a");
    REQUIRE(std::holds_alternative<DirectorDesk::Core::CancelAiJobCommand>(cancel));
    REQUIRE(std::get<DirectorDesk::Core::SetAiSettingsCommand>(settings).provider == "mock");
    REQUIRE(std::get<DirectorDesk::Core::RunSkillCommand>(run).assetId == "skill-1");
}

TEST_CASE("Undo Redo and ExportStoryboardPdf join the variant") {
    using DirectorDesk::Core::Command;
    Command undo = DirectorDesk::Core::UndoCommand{};
    Command redo = DirectorDesk::Core::RedoCommand{};
    Command pdf = DirectorDesk::Core::ExportStoryboardPdfCommand{};
    REQUIRE(std::holds_alternative<DirectorDesk::Core::UndoCommand>(undo));
    REQUIRE(std::holds_alternative<DirectorDesk::Core::RedoCommand>(redo));
    REQUIRE(std::holds_alternative<DirectorDesk::Core::ExportStoryboardPdfCommand>(pdf));
}
