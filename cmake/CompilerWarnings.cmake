# CompilerWarnings: CMake module for the DirectorDesk build configuration module.
# This file owns project behavior only; keep platform and dependency boundaries explicit.

# cmake/CompilerWarnings.cmake
# Treat warnings as errors for first-party targets only.

function(dd_set_strict_warnings target)
    if(MSVC)
        target_compile_options(${target} PRIVATE
            /W4
            /WX
            /permissive-
            /utf-8
            /Zc:__cplusplus
        )
    else()
        target_compile_options(${target} PRIVATE
            -Wall
            -Wextra
            -Wpedantic
            -Werror
            -Wconversion
            -Wshadow
        )
    endif()
endfunction()
