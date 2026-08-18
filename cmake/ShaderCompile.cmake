# Compile bgfx .sc shaders to per-renderer binaries. Outputs are never committed.

function(dd_find_shaderc)
    if(BGFX_SHADERC)
        return()
    endif()

    set(_search_dirs "")
    if(DEFINED VCPKG_INSTALLED_DIR AND DEFINED VCPKG_TARGET_TRIPLET)
        list(APPEND _search_dirs "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/tools/bgfx")
    endif()
    list(APPEND _search_dirs "${CMAKE_BINARY_DIR}/vcpkg_installed/${VCPKG_TARGET_TRIPLET}/tools/bgfx")

    find_program(BGFX_SHADERC NAMES shaderc shaderc.exe PATHS ${_search_dirs} NO_DEFAULT_PATH)
    if(NOT BGFX_SHADERC)
        find_program(BGFX_SHADERC NAMES shaderc shaderc.exe)
    endif()
    if(NOT BGFX_SHADERC)
        message(FATAL_ERROR "shaderc not found. Install bgfx with the tools feature.")
    endif()
endfunction()

function(dd_find_bgfx_shader_include)
    if(BGFX_SHADER_INCLUDE_DIR)
        return()
    endif()

    find_path(BGFX_SHADER_INCLUDE_DIR
        NAMES bgfx_shader.sh
        PATHS
            "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/include/bgfx"
            "${CMAKE_BINARY_DIR}/vcpkg_installed/${VCPKG_TARGET_TRIPLET}/include/bgfx"
        NO_DEFAULT_PATH
    )
    if(NOT BGFX_SHADER_INCLUDE_DIR)
        find_path(BGFX_SHADER_INCLUDE_DIR NAMES bgfx_shader.sh)
    endif()
    if(NOT BGFX_SHADER_INCLUDE_DIR)
        message(FATAL_ERROR "bgfx_shader.sh not found.")
    endif()
endfunction()

# dd_compile_bgfx_shader(NAME vs_mesh TYPE vertex SOURCE ... VARYING ... OUTPUT_DIR ...)
function(dd_compile_bgfx_shader)
    cmake_parse_arguments(SHADER "" "NAME;TYPE;SOURCE;VARYING;OUTPUT_DIR" "" ${ARGN})
    dd_find_shaderc()
    dd_find_bgfx_shader_include()

    set(_backends
        "dx11|windows|s_5_0"
        "metal|osx|metal"
        "glsl|linux|430"
        "spirv|linux|spirv"
    )

    set(_outputs "")
    foreach(_backend ${_backends})
        string(REPLACE "|" ";" _parts "${_backend}")
        list(GET _parts 0 _folder)
        list(GET _parts 1 _platform)
        list(GET _parts 2 _profile)
        set(_out_dir "${SHADER_OUTPUT_DIR}/${_folder}")
        set(_out_file "${_out_dir}/${SHADER_NAME}.bin")
        file(MAKE_DIRECTORY "${_out_dir}")
        add_custom_command(
            OUTPUT "${_out_file}"
            COMMAND "${BGFX_SHADERC}"
                -f "${SHADER_SOURCE}"
                -o "${_out_file}"
                --type "${SHADER_TYPE}"
                --platform "${_platform}"
                -p "${_profile}"
                --varyingdef "${SHADER_VARYING}"
                -i "${BGFX_SHADER_INCLUDE_DIR}"
            DEPENDS "${SHADER_SOURCE}" "${SHADER_VARYING}"
            COMMENT "shaderc ${SHADER_NAME} (${_folder})"
            VERBATIM
        )
        list(APPEND _outputs "${_out_file}")
    endforeach()
    set(${SHADER_NAME}_outputs "${_outputs}" PARENT_SCOPE)
endfunction()
