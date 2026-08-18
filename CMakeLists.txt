cmake_minimum_required(VERSION 3.24)

if(NOT DEFINED CMAKE_TOOLCHAIN_FILE AND DEFINED ENV{VCPKG_ROOT})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
        CACHE FILEPATH "vcpkg toolchain")
endif()

project(DirectorDesk VERSION 0.1.0 LANGUAGES CXX)

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

option(DD_BUILD_TESTS "Build DirectorDesk unit tests" ON)

find_package(spdlog CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(glad CONFIG REQUIRED)
find_package(OpenGL REQUIRED)

add_subdirectory(src/Core)
add_subdirectory(src/Platform)
add_subdirectory(src/UI)
add_subdirectory(backends)
add_subdirectory(src/App)

dd_add_interface_module(scene)
dd_add_interface_module(asset)
dd_add_interface_module(camera)
dd_add_interface_module(renderer)
dd_add_interface_module(export)
dd_add_interface_module(script)
dd_add_interface_module(link)
dd_add_interface_module(storyboard)
dd_add_interface_module(ai)

# App is the composition root and may depend on all modules.
target_link_libraries(dd_app
    PUBLIC
        DirectorDesk::scene
        DirectorDesk::asset
        DirectorDesk::camera
        DirectorDesk::renderer
        DirectorDesk::export
        DirectorDesk::script
        DirectorDesk::link
        DirectorDesk::storyboard
        DirectorDesk::ai
)

add_executable(DirectorDesk src/App/main.cpp)
target_link_libraries(DirectorDesk PRIVATE dd_app)
dd_set_strict_warnings(DirectorDesk)

if(DD_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
