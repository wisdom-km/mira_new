add_library(dd_backend_imgui
    imgui/ImGuiGlfwBackend.cpp
)
add_library(DirectorDesk::backend_imgui ALIAS dd_backend_imgui)

target_include_directories(dd_backend_imgui
    PUBLIC
        "${PROJECT_SOURCE_DIR}/backends/imgui"
)

target_link_libraries(dd_backend_imgui
    PUBLIC
        dd_platform
    PRIVATE
        imgui::imgui
        glfw
        glad::glad
        OpenGL::GL
)

target_compile_definitions(dd_backend_imgui PRIVATE GLFW_INCLUDE_NONE)

target_compile_features(dd_backend_imgui PUBLIC cxx_std_17)
dd_set_strict_warnings(dd_backend_imgui)
