add_library(dd_app
    Application.cpp
    ProjectFile.cpp
    ProjectBinding.cpp
)
add_library(DirectorDesk::app ALIAS dd_app)

target_include_directories(dd_app
    PUBLIC
        "${PROJECT_SOURCE_DIR}/include"
)

find_package(nlohmann_json CONFIG REQUIRED)

target_link_libraries(dd_app
    PUBLIC
        dd_core
        dd_platform
        dd_ui
        dd_camera
        dd_renderer
        dd_scene
        dd_asset
        dd_script
        dd_link
        dd_backend_imgui
        dd_backend_bgfx
        nlohmann_json::nlohmann_json
)

target_compile_features(dd_app PUBLIC cxx_std_17)
dd_set_strict_warnings(dd_app)
