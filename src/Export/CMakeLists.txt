# CMakeLists: Project source file for the DirectorDesk Export module.
# This file owns project behavior only; keep platform and dependency boundaries explicit.

add_library(dd_export
    ShotExport.cpp
)
add_library(DirectorDesk::export ALIAS dd_export)

target_include_directories(dd_export
    PUBLIC
        "${PROJECT_SOURCE_DIR}/include"
)

target_link_libraries(dd_export
    PUBLIC
        dd_core
        dd_renderer
)

target_compile_features(dd_export PUBLIC cxx_std_17)
dd_set_strict_warnings(dd_export)
