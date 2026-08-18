# CMakeLists: Project source file for the DirectorDesk Core module.
# This file owns project behavior only; keep platform and dependency boundaries explicit.

add_library(dd_core
    Error.cpp
    Log.cpp
    CommandQueue.cpp
    Sha256.cpp
)
add_library(DirectorDesk::core ALIAS dd_core)

find_path(PICOSHA2_INCLUDE_DIR picosha2.h)

target_include_directories(dd_core
    PUBLIC
        "${PROJECT_SOURCE_DIR}/include"
    PRIVATE
        "${PICOSHA2_INCLUDE_DIR}"
)

target_link_libraries(dd_core
    PUBLIC
        fmt::fmt
    PRIVATE
        spdlog::spdlog
)

target_compile_features(dd_core PUBLIC cxx_std_17)
dd_set_strict_warnings(dd_core)
