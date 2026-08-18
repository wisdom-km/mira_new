# Modules: CMake module for the DirectorDesk build configuration module.
# This file owns project behavior only; keep platform and dependency boundaries explicit.

function(dd_add_interface_module module_name)
    set(target_name "dd_${module_name}")
    add_library(${target_name} INTERFACE)
    add_library(DirectorDesk::${module_name} ALIAS ${target_name})
    target_include_directories(${target_name} INTERFACE
        "${PROJECT_SOURCE_DIR}/include"
    )
    target_link_libraries(${target_name} INTERFACE dd_core)
endfunction()
