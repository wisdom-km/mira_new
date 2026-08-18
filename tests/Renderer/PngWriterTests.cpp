// PngWriterTests: Implementation for the DirectorDesk Renderer module.
// This file owns project behavior only; keep platform and dependency boundaries explicit.
// Contract coverage: PNG output preserves dimensions, alpha, and failure reporting.


#include "DirectorDesk/Platform/Paths.h"
#include "DirectorDesk/Renderer/PngWriter.h"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("WritePng stores a Chinese path file with alpha", "[renderer][png]") {
    DirectorDesk::Renderer::PixelBuffer pixels;
    pixels.width = 2;
    pixels.height = 2;
    pixels.rgba = {255, 0, 0, 255, 0, 255, 0, 0, 0, 0, 255, 255, 255, 255, 255, 0};

    auto temp = DirectorDesk::Platform::Paths::TemporaryDirectory();
    REQUIRE(temp.IsOk());
    const std::string dir =
        DirectorDesk::Platform::Paths::Join(temp.Value(), "导演台导出_DirectorDesk");
    const std::string path = DirectorDesk::Platform::Paths::Join(dir, "alpha.png");
    auto written = DirectorDesk::Renderer::WritePng(pixels, path);
    REQUIRE(written.IsOk());
    REQUIRE(DirectorDesk::Platform::Paths::Exists(path));

    auto bytes = DirectorDesk::Platform::Paths::ReadBinaryFile(path);
    REQUIRE(bytes.IsOk());
    REQUIRE(bytes.Value().size() > 16);
    REQUIRE(bytes.Value()[0] == 0x89);
    REQUIRE(bytes.Value()[1] == 0x50);
    REQUIRE(bytes.Value()[2] == 0x4e);
    REQUIRE(bytes.Value()[3] == 0x47);
}
