# CMakeLists: Project source file for the DirectorDesk Script module.
# This file owns project behavior only; keep platform and dependency boundaries explicit.

add_library(dd_script
    Ids.cpp
    Parser.cpp
    Document.cpp
)
add_library(DirectorDesk::script ALIAS dd_script)

target_include_directories(dd_script
    PUBLIC
        "${PROJECT_SOURCE_DIR}/include"
)

target_link_libraries(dd_script
    PUBLIC
        dd_core
        dd_platform
)

target_compile_features(dd_script PUBLIC cxx_std_17)
dd_set_strict_warnings(dd_script)
