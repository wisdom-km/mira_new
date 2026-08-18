# CMakeLists: Project source file for the DirectorDesk AI module.
# This file owns project behavior only; keep platform and dependency boundaries explicit.

add_library(dd_ai
    Types.cpp
    NullServices.cpp
    MockServices.cpp
)
add_library(DirectorDesk::ai ALIAS dd_ai)

target_include_directories(dd_ai
    PUBLIC
        "${PROJECT_SOURCE_DIR}/include"
)

target_link_libraries(dd_ai
    PUBLIC
        dd_core
)

target_compile_features(dd_ai PUBLIC cxx_std_17)
dd_set_strict_warnings(dd_ai)
