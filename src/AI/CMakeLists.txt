# CMakeLists: Project source file for the DirectorDesk AI module.

add_library(dd_ai
    Types.cpp
    NullServices.cpp
    MockServices.cpp
    HttpCompatible.cpp
    SkillRunner.cpp
)
add_library(DirectorDesk::ai ALIAS dd_ai)

find_package(nlohmann_json CONFIG REQUIRED)

target_include_directories(dd_ai
    PUBLIC
        "${PROJECT_SOURCE_DIR}/include"
)

target_link_libraries(dd_ai
    PUBLIC
        dd_core
        dd_platform
    PRIVATE
        nlohmann_json::nlohmann_json
)

target_compile_features(dd_ai PUBLIC cxx_std_17)
dd_set_strict_warnings(dd_ai)
