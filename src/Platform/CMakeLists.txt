add_library(dd_platform
    Paths.cpp
    Startup.cpp
    "${PROJECT_SOURCE_DIR}/backends/glfw/GlfwWindow.cpp"
)
add_library(DirectorDesk::platform ALIAS dd_platform)

target_include_directories(dd_platform
    PUBLIC
        "${PROJECT_SOURCE_DIR}/include"
)

target_link_libraries(dd_platform
    PUBLIC
        dd_core
    PRIVATE
        glfw
)

target_compile_definitions(dd_platform PRIVATE GLFW_INCLUDE_NONE)

if(WIN32)
    target_link_libraries(dd_platform PRIVATE ole32 shell32)
endif()

target_compile_features(dd_platform PUBLIC cxx_std_17)
dd_set_strict_warnings(dd_platform)
