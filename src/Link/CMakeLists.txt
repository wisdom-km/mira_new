# CMakeLists: Project source file for the DirectorDesk Link module.
# This file owns project behavior only; keep platform and dependency boundaries explicit.

add_library(dd_link
    ShotLink.cpp
)
add_library(DirectorDesk::link ALIAS dd_link)

target_include_directories(dd_link
    PUBLIC
        "${PROJECT_SOURCE_DIR}/include"
)

target_link_libraries(dd_link
    PUBLIC
        dd_core
)

target_compile_features(dd_link PUBLIC cxx_std_17)
dd_set_strict_warnings(dd_link)
