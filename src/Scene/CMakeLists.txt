add_library(dd_scene
    Transform.cpp
    Document.cpp
)
add_library(DirectorDesk::scene ALIAS dd_scene)

target_include_directories(dd_scene
    PUBLIC
        "${PROJECT_SOURCE_DIR}/include"
)

target_link_libraries(dd_scene
    PUBLIC
        dd_core
        glm::glm
)

target_compile_features(dd_scene PUBLIC cxx_std_17)
dd_set_strict_warnings(dd_scene)
