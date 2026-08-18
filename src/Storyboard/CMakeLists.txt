# CMakeLists: Project source file for the DirectorDesk Storyboard module.
# This file owns project behavior only; keep platform and dependency boundaries explicit.

add_library(dd_storyboard
    Layout.cpp
    Document.cpp
    BoardComposer.cpp
)
add_library(DirectorDesk::storyboard ALIAS dd_storyboard)

target_include_directories(dd_storyboard
    PUBLIC
        "${PROJECT_SOURCE_DIR}/include"
)

find_path(STB_INCLUDE_DIR stb_truetype.h)
target_include_directories(dd_storyboard PRIVATE "${STB_INCLUDE_DIR}")

target_link_libraries(dd_storyboard
    PUBLIC
        dd_core
        dd_platform
)

target_compile_features(dd_storyboard PUBLIC cxx_std_17)
dd_set_strict_warnings(dd_storyboard)
