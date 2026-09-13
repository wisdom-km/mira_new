// ImageDecodeTests: Coverage for UIC-20 sidecar / official preview decode.
// This file owns project behavior only; keep platform and dependency boundaries explicit.

#include "DirectorDesk/Asset/ImageDecode.h"
#include "DirectorDesk/Platform/Paths.h"
#include "DirectorDesk/Renderer/PngWriter.h"
#include "DirectorDesk/UI/IPanel.h"

#include <catch2/catch_test_macros.hpp>

#include <cstdint>
#include <string>

TEST_CASE("LibraryAssetView previewTexture defaults to invalid", "[ui][library]") {
    const DirectorDesk::UI::LibraryAssetView item;
    REQUIRE(item.previewTexture == 0xFFFFu);
}

TEST_CASE("DecodeImageFile fails on a missing path", "[asset][image]") {
    auto decoded = DirectorDesk::Asset::DecodeImageFile("Z:/does-not-exist/preview.png");
    REQUIRE_FALSE(decoded.IsOk());
}

TEST_CASE("DecodeImageMemory fails on an empty buffer", "[asset][image]") {
    auto decoded = DirectorDesk::Asset::DecodeImageMemory(nullptr, 0);
    REQUIRE_FALSE(decoded.IsOk());
}

TEST_CASE("DecodeImageFile roundtrips a Chinese path PNG", "[asset][image]") {
    DirectorDesk::Renderer::PixelBuffer pixels;
    pixels.width = 2;
    pixels.height = 2;
    pixels.rgba = {255, 0, 0, 255, 0, 255, 0, 128, 0, 0, 255, 255, 255, 255, 255, 0};

    auto temp = DirectorDesk::Platform::Paths::TemporaryDirectory();
    REQUIRE(temp.IsOk());
    const std::string dir =
        DirectorDesk::Platform::Paths::Join(temp.Value(), "导演台预览_DirectorDesk");
    const std::string path = DirectorDesk::Platform::Paths::Join(dir, "sidecar.png");
    REQUIRE(DirectorDesk::Renderer::WritePng(pixels, path).IsOk());

    auto decoded = DirectorDesk::Asset::DecodeImageFile(path);
    REQUIRE(decoded.IsOk());
    REQUIRE(decoded.Value().width == 2);
    REQUIRE(decoded.Value().height == 2);
    REQUIRE(decoded.Value().rgba.size() == 16);
    REQUIRE(decoded.Value().rgba[0] == 255);
    REQUIRE(decoded.Value().rgba[1] == 0);
    REQUIRE(decoded.Value().rgba[2] == 0);
    REQUIRE(decoded.Value().rgba[3] == 255);
}
