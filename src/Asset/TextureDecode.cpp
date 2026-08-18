// TextureDecode: Implementation for the DirectorDesk Asset module.
// This file owns project behavior only; keep platform and dependency boundaries explicit.

#define STB_IMAGE_IMPLEMENTATION
#include "TextureDecode.h"

#include "DirectorDesk/Platform/Paths.h"

#include <stb_image.h>

namespace DirectorDesk::Asset {

Core::Result<DecodedImage> DecodeImageMemory(const std::uint8_t* data, std::size_t size) {
    if (data == nullptr || size == 0) {
        return Core::Result<DecodedImage>::Fail(Core::Error::Make(
            Core::ErrorCode::InvalidArgument, "Image buffer is empty", "纹理数据无效"));
    }

    int width = 0;
    int height = 0;
    int channels = 0;
    stbi_uc* pixels =
        stbi_load_from_memory(data, static_cast<int>(size), &width, &height, &channels, 4);
    if (pixels == nullptr || width <= 0 || height <= 0) {
        return Core::Result<DecodedImage>::Fail(Core::Error::Make(
            Core::ErrorCode::ParseFailure, "stbi_load_from_memory failed", "无法解码纹理"));
    }

    DecodedImage image;
    image.width = static_cast<std::uint32_t>(width);
    image.height = static_cast<std::uint32_t>(height);
    image.rgba.assign(pixels, pixels + (static_cast<std::size_t>(width) * height * 4));
    stbi_image_free(pixels);
    return Core::Result<DecodedImage>::Ok(std::move(image));
}

Core::Result<DecodedImage> DecodeImageFile(const std::string& utf8Path) {
    auto bytes = Platform::Paths::ReadBinaryFile(utf8Path);
    if (!bytes.IsOk()) {
        return Core::Result<DecodedImage>::Fail(bytes.GetError());
    }
    return DecodeImageMemory(bytes.Value().data(), bytes.Value().size());
}

} // namespace DirectorDesk::Asset
