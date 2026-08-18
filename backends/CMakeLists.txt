# CMakeLists: Project source file for the DirectorDesk CMakeLists.txt module.
# This file owns project behavior only; keep platform and dependency boundaries explicit.

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
        bgfx::bgfx
        glm::glm
)

target_compile_definitions(dd_backend_imgui PRIVATE GLFW_INCLUDE_NONE)
target_compile_features(dd_backend_imgui PUBLIC cxx_std_17)
dd_set_strict_warnings(dd_backend_imgui)

add_library(dd_backend_bgfx
    bgfx/BgfxRenderer.cpp
)
add_library(DirectorDesk::backend_bgfx ALIAS dd_backend_bgfx)

target_include_directories(dd_backend_bgfx
    PUBLIC
        "${PROJECT_SOURCE_DIR}/backends/bgfx"
)

target_link_libraries(dd_backend_bgfx
    PUBLIC
        dd_renderer
    PRIVATE
        dd_platform
        bgfx::bgfx
        glm::glm
)

target_compile_features(dd_backend_bgfx PUBLIC cxx_std_17)
dd_set_strict_warnings(dd_backend_bgfx)

find_package(CURL REQUIRED)
add_library(dd_backend_curl
    curl/CurlHttpClient.cpp
)
add_library(DirectorDesk::backend_curl ALIAS dd_backend_curl)
target_include_directories(dd_backend_curl
    PUBLIC
        "${PROJECT_SOURCE_DIR}/backends/curl"
)
target_link_libraries(dd_backend_curl
    PUBLIC
        dd_platform
    PRIVATE
        CURL::libcurl
)
target_compile_features(dd_backend_curl PUBLIC cxx_std_17)
dd_set_strict_warnings(dd_backend_curl)
