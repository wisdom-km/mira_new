// Sha256: Public or internal interface for the DirectorDesk Core module.
// This file owns project behavior only; keep platform and dependency boundaries explicit.

#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace DirectorDesk::Core {

[[nodiscard]] std::string Sha256Hex(const std::uint8_t* data, std::size_t size);
[[nodiscard]] std::string Sha256Hex(const std::vector<std::uint8_t>& data);

} // namespace DirectorDesk::Core
