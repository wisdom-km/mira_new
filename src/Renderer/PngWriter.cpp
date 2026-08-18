#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "DirectorDesk/Renderer/PngWriter.h"

#include "DirectorDesk/Platform/Paths.h"

#include <stb_image_write.h>

#include <vector>

namespace DirectorDesk::Renderer {
namespace {

void PngWriteCallback(void* context, void* data, int size) {
    auto* bytes = static_cast<std::vector<std::uint8_t>*>(context);
    const auto* begin = static_cast<const std::uint8_t*>(data);
    bytes->insert(bytes->end(), begin, begin + size);
}

} // namespace

Core::Result<void> WritePng(const PixelBuffer& pixels, const std::string& utf8Path) {
    if (pixels.width == 0 || pixels.height == 0 ||
        pixels.rgba.size() != static_cast<std::size_t>(pixels.width) * pixels.height * 4u) {
        return Core::Result<void>::Fail(Core::Error::Make(
            Core::ErrorCode::InvalidArgument, "PixelBuffer size is invalid", "图像数据无效"));
    }

    std::vector<std::uint8_t> pngBytes;
    const int stride = static_cast<int>(pixels.width * 4u);
    if (stbi_write_png_to_func(&PngWriteCallback, &pngBytes, static_cast<int>(pixels.width),
                               static_cast<int>(pixels.height), 4, pixels.rgba.data(),
                               stride) == 0) {
        return Core::Result<void>::Fail(Core::Error::Make(
            Core::ErrorCode::IoFailure, "stbi_write_png_to_func failed", "无法编码 PNG"));
    }

    return Platform::Paths::WriteBinaryFile(utf8Path, pngBytes.data(), pngBytes.size());
}

} // namespace DirectorDesk::Renderer
