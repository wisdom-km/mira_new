# CMakeLists: Project source file for the DirectorDesk CMakeLists.txt module.
# This file owns project behavior only; keep platform and dependency boundaries explicit.

find_package(Catch2 CONFIG REQUIRED)

add_executable(DirectorDeskTests
    Core/ResultTests.cpp
    Core/LogTests.cpp
    Platform/PathTests.cpp
    Camera/OrbitCameraTests.cpp
    Camera/PresetTests.cpp
    Camera/CameraManagerTests.cpp
    Renderer/PngWriterTests.cpp
    Asset/ModelLoaderTests.cpp
    Asset/LibraryTests.cpp
    Asset/ManifestTests.cpp
    Asset/OfficialCatalogTests.cpp
    Scene/TransformTests.cpp
    Script/ParserTests.cpp
    Script/DocumentTests.cpp
    Link/ShotLinkTests.cpp
    App/ProjectFileTests.cpp
    Storyboard/LayoutTests.cpp
    Export/ShotExportTests.cpp
    AI/ServiceTests.cpp
)

target_include_directories(DirectorDeskTests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")

target_link_libraries(DirectorDeskTests
    PRIVATE
        dd_core
        dd_platform
        dd_camera
        dd_renderer
        dd_asset
        dd_scene
        dd_script
        dd_link
        dd_app
        dd_storyboard
        dd_export
        dd_ai
        Catch2::Catch2WithMain
)

dd_set_strict_warnings(DirectorDeskTests)
target_compile_definitions(DirectorDeskTests PRIVATE
    "DD_EXAMPLE_CUBE_GLB=\"${PROJECT_SOURCE_DIR}/examples/models/cube.glb\""
    "DD_EXAMPLE_CAFE_SCRIPT=\"${PROJECT_SOURCE_DIR}/examples/scripts/cafe.md\""
    "DD_EXAMPLE_CAFE_PROJECT=\"${PROJECT_SOURCE_DIR}/examples/cafe.ddproj\""
)

add_test(NAME DirectorDeskTests COMMAND DirectorDeskTests)
