# CMakeLists: Project source file for the DirectorDesk Camera module.
# This file owns project behavior only; keep platform and dependency boundaries explicit.

add_library(dd_camera
    OrbitCamera.cpp
    Presets.cpp
    CameraManager.cpp
)
add_library(DirectorDesk::camera ALIAS dd_camera)

target_include_directories(dd_camera
    PUBLIC
        "${PROJECT_SOURCE_DIR}/include"
)

target_link_libraries(dd_camera
    PUBLIC
        dd_core
        glm::glm
)

target_compile_features(dd_camera PUBLIC cxx_std_17)
dd_set_strict_warnings(dd_camera)
