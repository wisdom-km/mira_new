// Sha256: Implementation for the DirectorDesk Core module.
// This file owns project behavior only; keep platform and dependency boundaries explicit.

#include "DirectorDesk/Core/Sha256.h"

#include <picosha2.h>

namespace DirectorDesk::Core {

std::string Sha256Hex(const std::uint8_t* data, std::size_t size) {
    if (data == nullptr || size == 0) {
        return picosha2::hash256_hex_string(std::vector<std::uint8_t>{});
    }
    return picosha2::hash256_hex_string(data, data + size);
}

std::string Sha256Hex(const std::vector<std::uint8_t>& data) {
    return picosha2::hash256_hex_string(data);
}

} // namespace DirectorDesk::Core
