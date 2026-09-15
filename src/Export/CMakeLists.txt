# CMakeLists: Project source file for the DirectorDesk Export module.
# This file owns project behavior only; keep platform and dependency boundaries explicit.

add_library(dd_export
    ShotExport.cpp
    BoardPdf.cpp
)
add_library(DirectorDesk::export ALIAS dd_export)

target_include_directories(dd_export
    PUBLIC
        "${PROJECT_SOURCE_DIR}/include"
)

find_package(nlohmann_json CONFIG REQUIRED)

target_link_libraries(dd_export
    PUBLIC
        dd_core
        dd_renderer
    PRIVATE
        dd_platform
        nlohmann_json::nlohmann_json
)

target_compile_features(dd_export PUBLIC cxx_std_17)
dd_set_strict_warnings(dd_export)
target_compile_definitions(dd_export PRIVATE "DD_PROJECT_VERSION=\"${PROJECT_VERSION}\"")
