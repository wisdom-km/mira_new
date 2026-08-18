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
    Scene/TransformTests.cpp
    Script/ParserTests.cpp
    Script/DocumentTests.cpp
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
        Catch2::Catch2WithMain
)

dd_set_strict_warnings(DirectorDeskTests)
target_compile_definitions(DirectorDeskTests PRIVATE
    "DD_EXAMPLE_CUBE_GLB=\"${PROJECT_SOURCE_DIR}/examples/models/cube.glb\""
    "DD_EXAMPLE_CAFE_SCRIPT=\"${PROJECT_SOURCE_DIR}/examples/scripts/cafe.md\""
)

add_test(NAME DirectorDeskTests COMMAND DirectorDeskTests)
