// TestModels: Public or internal interface for the DirectorDesk Asset module.
// This file owns project behavior only; keep platform and dependency boundaries explicit.

#pragma once

#include "DirectorDesk/Platform/Paths.h"

#include <cstddef>
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

inline void AppendLe32(std::vector<std::uint8_t>& out, std::uint32_t value) {
    out.push_back(static_cast<std::uint8_t>(value));
    out.push_back(static_cast<std::uint8_t>(value >> 8));
    out.push_back(static_cast<std::uint8_t>(value >> 16));
    out.push_back(static_cast<std::uint8_t>(value >> 24));
}

inline void AppendBytes(std::vector<std::uint8_t>& out, const void* data, std::size_t size) {
    const auto* bytes = static_cast<const std::uint8_t*>(data);
    out.insert(out.end(), bytes, bytes + size);
}

inline void PadTo4(std::vector<std::uint8_t>& out, std::uint8_t pad) {
    while ((out.size() % 4) != 0) {
        out.push_back(pad);
    }
}

inline std::string WriteGlbFile(const std::string& path, std::string json,
                                std::vector<std::uint8_t> bin) {
    while ((json.size() % 4) != 0) {
        json.push_back(' ');
    }
    PadTo4(bin, 0);
    std::vector<std::uint8_t> glb;
    const std::uint32_t jsonLength = static_cast<std::uint32_t>(json.size());
    const std::uint32_t binLength = static_cast<std::uint32_t>(bin.size());
    const std::uint32_t total = 12 + 8 + jsonLength + 8 + binLength;
    AppendLe32(glb, 0x46546C67u);
    AppendLe32(glb, 2);
    AppendLe32(glb, total);
    AppendLe32(glb, jsonLength);
    AppendLe32(glb, 0x4E4F534Au);
    AppendBytes(glb, json.data(), json.size());
    AppendLe32(glb, binLength);
    AppendLe32(glb, 0x004E4942u);
    AppendBytes(glb, bin.data(), bin.size());
    DirectorDesk::Platform::Paths::WriteBinaryFile(path, glb.data(), glb.size());
    return path;
}

inline std::string WriteSkinnedTriangleGlb(const std::string& directory) {
    const float positions[] = {-1.0f, -1.0f, 0.0f, 1.0f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f};
    const std::uint16_t indices[] = {0, 1, 2};
    const float inverseBind[] = {1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
                                 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f};
    std::vector<std::uint8_t> bin;
    AppendBytes(bin, positions, sizeof(positions));
    AppendBytes(bin, indices, sizeof(indices));
    while ((bin.size() % 4) != 0) {
        bin.push_back(0);
    }
    AppendBytes(bin, inverseBind, sizeof(inverseBind));
    const std::string json =
        "{\"asset\":{\"version\":\"2.0\"},\"scene\":0,\"scenes\":[{\"nodes\":[0]}],"
        "\"nodes\":[{\"mesh\":0,\"skin\":0,\"children\":[1]},{\"translation\":[0,1,0]}],"
        "\"meshes\":[{\"primitives\":[{\"attributes\":{\"POSITION\":0},\"indices\":1}]}],"
        "\"skins\":[{\"joints\":[1],\"inverseBindMatrices\":2}],"
        "\"accessors\":["
        "{\"bufferView\":0,\"componentType\":5126,\"count\":3,\"type\":\"VEC3\","
        "\"max\":[1,1,0],\"min\":[-1,-1,0]},"
        "{\"bufferView\":1,\"componentType\":5123,\"count\":3,\"type\":\"SCALAR\"},"
        "{\"bufferView\":2,\"componentType\":5126,\"count\":1,\"type\":\"MAT4\"}],"
        "\"bufferViews\":["
        "{\"buffer\":0,\"byteOffset\":0,\"byteLength\":36},"
        "{\"buffer\":0,\"byteOffset\":36,\"byteLength\":6},"
        "{\"buffer\":0,\"byteOffset\":44,\"byteLength\":64}],"
        "\"buffers\":[{\"byteLength\":108}]}";
    const std::string path = DirectorDesk::Platform::Paths::Join(directory, "skinned.glb");
    return WriteGlbFile(path, json, std::move(bin));
}

inline std::string WriteStaticTriangleGlb(const std::string& directory) {
    const float positions[] = {-1.0f, -1.0f, 0.0f, 1.0f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f};
    const std::uint16_t indices[] = {0, 1, 2};
    std::vector<std::uint8_t> bin;
    AppendBytes(bin, positions, sizeof(positions));
    AppendBytes(bin, indices, sizeof(indices));
    const std::string json =
        "{\"asset\":{\"version\":\"2.0\"},\"scene\":0,\"scenes\":[{\"nodes\":[0]}],"
        "\"nodes\":[{\"mesh\":0}],"
        "\"meshes\":[{\"primitives\":[{\"attributes\":{\"POSITION\":0},\"indices\":1}]}],"
        "\"accessors\":["
        "{\"bufferView\":0,\"componentType\":5126,\"count\":3,\"type\":\"VEC3\","
        "\"max\":[1,1,0],\"min\":[-1,-1,0]},"
        "{\"bufferView\":1,\"componentType\":5123,\"count\":3,\"type\":\"SCALAR\"}],"
        "\"bufferViews\":["
        "{\"buffer\":0,\"byteOffset\":0,\"byteLength\":36},"
        "{\"buffer\":0,\"byteOffset\":36,\"byteLength\":6}],"
        "\"buffers\":[{\"byteLength\":42}]}";
    const std::string path = DirectorDesk::Platform::Paths::Join(directory, "static.glb");
    return WriteGlbFile(path, json, std::move(bin));
}

} // namespace DirectorDesk::Tests
