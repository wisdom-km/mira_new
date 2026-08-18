#pragma once

#include "DirectorDesk/Platform/Paths.h"

#include <cstdint>
#include <string>
#include <vector>

namespace DirectorDesk::Tests {

inline std::string WriteCubeObj(const std::string& directory, bool withNormals) {
    const std::string objPath = DirectorDesk::Platform::Paths::Join(directory, "cube.obj");
    std::string text = "o Cube\n"
                       "v -0.5 -0.5 0.5\n"
                       "v  0.5 -0.5 0.5\n"
                       "v  0.5  0.5 0.5\n"
                       "f 1 2 3\n";
    if (withNormals) {
        text = "o Cube\n"
               "v -0.5 -0.5 0.5\n"
               "v  0.5 -0.5 0.5\n"
               "v  0.5  0.5 0.5\n"
               "vn 0 0 1\n"
               "f 1//1 2//1 3//1\n";
    }
    const auto written = DirectorDesk::Platform::Paths::WriteBinaryFile(
        objPath, reinterpret_cast<const std::uint8_t*>(text.data()), text.size());
    (void)written;
    return objPath;
}

inline std::string WriteTextFile(const std::string& path, const std::string& text) {
    DirectorDesk::Platform::Paths::WriteBinaryFile(
        path, reinterpret_cast<const std::uint8_t*>(text.data()), text.size());
    return path;
}

} // namespace DirectorDesk::Tests
