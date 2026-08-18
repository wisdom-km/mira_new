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
        dd_storyboard
        dd_export
        dd_backend_imgui
        dd_backend_bgfx
        dd_backend_curl
        nlohmann_json::nlohmann_json
)

if(NOT DD_OFFICIAL_MANIFEST_URL)
    set(DD_OFFICIAL_MANIFEST_URL
        "https://raw.githubusercontent.com/wisdom-km/obj-3d-models/main/manifest.json")
endif()
if(NOT DD_OFFICIAL_ASSET_BASE_URL)
    set(DD_OFFICIAL_ASSET_BASE_URL
        "https://raw.githubusercontent.com/wisdom-km/obj-3d-models/main/")
endif()
target_compile_definitions(dd_app PRIVATE
    "DD_OFFICIAL_MANIFEST_URL=\"${DD_OFFICIAL_MANIFEST_URL}\""
    "DD_OFFICIAL_ASSET_BASE_URL=\"${DD_OFFICIAL_ASSET_BASE_URL}\""
)

target_compile_features(dd_app PUBLIC cxx_std_17)
dd_set_strict_warnings(dd_app)
