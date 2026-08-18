#pragma once

#include "DirectorDesk/Core/Result.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace DirectorDesk::Asset {

struct DecodedImage {
    std::uint32_t width = 0;
    std::uint32_t height = 0;
    std::vector<std::uint8_t> rgba;
};

Core::Result<DecodedImage> DecodeImageMemory(const std::uint8_t* data, std::size_t size);
Core::Result<DecodedImage> DecodeImageFile(const std::string& utf8Path);

} // namespace DirectorDesk::Asset
