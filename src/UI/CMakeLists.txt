add_library(dd_ui
    WorkspacePanel.cpp
    ScriptPanel.cpp
    LibraryPanel.cpp
    StoryboardPanel.cpp
)
add_library(DirectorDesk::ui ALIAS dd_ui)

target_include_directories(dd_ui
    PUBLIC
        "${PROJECT_SOURCE_DIR}/include"
)

# UI may use Dear ImGui. It must not link Platform, GLFW, spdlog, or bgfx.
target_link_libraries(dd_ui
    PUBLIC
        dd_core
    PRIVATE
        imgui::imgui
)

target_compile_features(dd_ui PUBLIC cxx_std_17)
dd_set_strict_warnings(dd_ui)
