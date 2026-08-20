# Root build graph: configures the C++17 application, backend libraries, tests, and shader tools.
cmake_minimum_required(VERSION 3.24)

if(NOT DEFINED CMAKE_TOOLCHAIN_FILE AND DEFINED ENV{VCPKG_ROOT})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
        CACHE FILEPATH "vcpkg toolchain")
endif()

project(DirectorDesk VERSION 0.1.2 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Build type" FORCE)
endif()

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")
include(CompilerWarnings)
include(Modules)
include(ShaderCompile)

option(DD_BUILD_TESTS "Build DirectorDesk unit tests" ON)
set(DD_OFFICIAL_MANIFEST_URL
    "https://raw.githubusercontent.com/wisdom-km/obj-3d-models/main/manifest.json"
    CACHE STRING "Official HTTPS manifest URL")
set(DD_OFFICIAL_ASSET_BASE_URL
    "https://raw.githubusercontent.com/wisdom-km/obj-3d-models/main/"
    CACHE STRING "Official HTTPS asset base URL")
if(DD_OFFICIAL_MANIFEST_URL STREQUAL "")
    set(DD_OFFICIAL_MANIFEST_URL
        "https://raw.githubusercontent.com/wisdom-km/obj-3d-models/main/manifest.json")
endif()
if(DD_OFFICIAL_ASSET_BASE_URL STREQUAL "")
    set(DD_OFFICIAL_ASSET_BASE_URL
        "https://raw.githubusercontent.com/wisdom-km/obj-3d-models/main/")
endif()

find_package(spdlog CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(bgfx CONFIG REQUIRED)

set(DD_SHADER_OUTPUT_DIR "${CMAKE_BINARY_DIR}/generated-shaders")
dd_compile_bgfx_shader(
    NAME vs_mesh
    TYPE vertex
    SOURCE "${PROJECT_SOURCE_DIR}/shaders/mesh/vs_mesh.sc"
    VARYING "${PROJECT_SOURCE_DIR}/shaders/mesh/varying.def.sc"
    OUTPUT_DIR "${DD_SHADER_OUTPUT_DIR}"
)
dd_compile_bgfx_shader(
    NAME fs_mesh
    TYPE fragment
    SOURCE "${PROJECT_SOURCE_DIR}/shaders/mesh/fs_mesh.sc"
    VARYING "${PROJECT_SOURCE_DIR}/shaders/mesh/varying.def.sc"
    OUTPUT_DIR "${DD_SHADER_OUTPUT_DIR}"
)
dd_compile_bgfx_shader(
    NAME vs_imgui
    TYPE vertex
    SOURCE "${PROJECT_SOURCE_DIR}/shaders/imgui/vs_imgui.sc"
    VARYING "${PROJECT_SOURCE_DIR}/shaders/imgui/varying.def.sc"
    OUTPUT_DIR "${DD_SHADER_OUTPUT_DIR}"
)
dd_compile_bgfx_shader(
    NAME fs_imgui
    TYPE fragment
    SOURCE "${PROJECT_SOURCE_DIR}/shaders/imgui/fs_imgui.sc"
    VARYING "${PROJECT_SOURCE_DIR}/shaders/imgui/varying.def.sc"
    OUTPUT_DIR "${DD_SHADER_OUTPUT_DIR}"
)

add_custom_target(dd_shaders ALL
    DEPENDS ${vs_mesh_outputs} ${fs_mesh_outputs} ${vs_imgui_outputs} ${fs_imgui_outputs}
)

add_subdirectory(src/Core)
add_subdirectory(src/Platform)
add_subdirectory(src/Camera)
add_subdirectory(src/Renderer)
add_subdirectory(src/Scene)
add_subdirectory(src/Asset)
add_subdirectory(src/Script)
add_subdirectory(src/UI)
add_subdirectory(src/Link)
add_subdirectory(src/Storyboard)
add_subdirectory(src/Export)
add_subdirectory(src/AI)
add_subdirectory(backends)
add_subdirectory(src/App)

target_link_libraries(dd_app
    PUBLIC
        DirectorDesk::scene
        DirectorDesk::asset
        DirectorDesk::export
        DirectorDesk::script
        DirectorDesk::link
        DirectorDesk::storyboard
        DirectorDesk::ai
)

add_executable(DirectorDesk src/App/main.cpp)
target_link_libraries(DirectorDesk PRIVATE dd_app)
dd_set_strict_warnings(DirectorDesk)
add_dependencies(DirectorDesk dd_shaders)

if(WIN32)
    # GLFW loads the first ICON named GLFW_ICON from the executable.
    file(TO_NATIVE_PATH "${PROJECT_SOURCE_DIR}/img/directordesk.ico" DD_APP_ICON)
    string(REPLACE "\\" "\\\\" DD_APP_ICON "${DD_APP_ICON}")
    configure_file(
        "${PROJECT_SOURCE_DIR}/packaging/windows/DirectorDesk.rc.in"
        "${CMAKE_BINARY_DIR}/DirectorDesk.rc"
        @ONLY
    )
    target_sources(DirectorDesk PRIVATE "${CMAKE_BINARY_DIR}/DirectorDesk.rc")
endif()

add_custom_command(TARGET DirectorDesk POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E copy_directory
        "${DD_SHADER_OUTPUT_DIR}"
        "$<TARGET_FILE_DIR:DirectorDesk>/shaders"
    COMMAND "${CMAKE_COMMAND}" -E copy_directory
        "${PROJECT_SOURCE_DIR}/examples"
        "$<TARGET_FILE_DIR:DirectorDesk>/examples"
    COMMENT "Copy compiled shaders and examples next to DirectorDesk"
)

if(DD_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
