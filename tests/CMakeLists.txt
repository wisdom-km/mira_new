find_package(Catch2 CONFIG REQUIRED)

add_executable(DirectorDeskTests
    Core/ResultTests.cpp
    Core/LogTests.cpp
    Platform/PathTests.cpp
    Camera/OrbitCameraTests.cpp
    Renderer/PngWriterTests.cpp
    Asset/ModelLoaderTests.cpp
    Scene/TransformTests.cpp
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
        Catch2::Catch2WithMain
)

dd_set_strict_warnings(DirectorDeskTests)
target_compile_definitions(DirectorDeskTests PRIVATE
    "DD_EXAMPLE_CUBE_GLB=\"${PROJECT_SOURCE_DIR}/examples/models/cube.glb\""
)

add_test(NAME DirectorDeskTests COMMAND DirectorDeskTests)
