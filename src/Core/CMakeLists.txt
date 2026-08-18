add_library(dd_core
    Error.cpp
    Log.cpp
    CommandQueue.cpp
)
add_library(DirectorDesk::core ALIAS dd_core)

target_include_directories(dd_core
    PUBLIC
        "${PROJECT_SOURCE_DIR}/include"
)

target_link_libraries(dd_core
    PUBLIC
        fmt::fmt
    PRIVATE
        spdlog::spdlog
)

target_compile_features(dd_core PUBLIC cxx_std_17)
dd_set_strict_warnings(dd_core)
