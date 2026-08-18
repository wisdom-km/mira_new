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
