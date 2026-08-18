# CMakeLists: Project source file for the DirectorDesk Renderer module.
# This file owns project behavior only; keep platform and dependency boundaries explicit.

add_library(dd_renderer
    PngWriter.cpp
)
add_library(DirectorDesk::renderer ALIAS dd_renderer)

target_include_directories(dd_renderer
    PUBLIC
        "${PROJECT_SOURCE_DIR}/include"
)

find_path(STB_INCLUDE_DIR stb_image_write.h)
target_include_directories(dd_renderer PRIVATE "${STB_INCLUDE_DIR}")

target_link_libraries(dd_renderer
    PUBLIC
        dd_core
        dd_platform
        glm::glm
)

target_compile_features(dd_renderer PUBLIC cxx_std_17)
dd_set_strict_warnings(dd_renderer)
