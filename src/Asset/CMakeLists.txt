add_library(dd_asset
    LoaderRegistry.cpp
    GlbLoader.cpp
    ObjLoader.cpp
    MeshUtils.cpp
    TextureDecode.cpp
    Library.cpp
)
add_library(DirectorDesk::asset ALIAS dd_asset)

target_include_directories(dd_asset
    PUBLIC
        "${PROJECT_SOURCE_DIR}/include"
    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}"
)

find_path(CGLTF_INCLUDE_DIR cgltf.h)
find_path(STB_INCLUDE_DIR stb_image.h)
find_package(tinyobjloader CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

target_include_directories(dd_asset PRIVATE "${CGLTF_INCLUDE_DIR}" "${STB_INCLUDE_DIR}")

target_link_libraries(dd_asset
    PUBLIC
        dd_core
        dd_platform
        glm::glm
        nlohmann_json::nlohmann_json
    PRIVATE
        tinyobjloader::tinyobjloader
)

target_compile_features(dd_asset PUBLIC cxx_std_17)
dd_set_strict_warnings(dd_asset)
if(MSVC)
    target_compile_definitions(dd_asset PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()
