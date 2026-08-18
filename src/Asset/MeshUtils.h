// MeshUtils: Public or internal interface for the DirectorDesk Asset module.
// This file owns project behavior only; keep platform and dependency boundaries explicit.

#pragma once

#include "DirectorDesk/Asset/ModelData.h"

#include <cstdint>
#include <vector>

namespace DirectorDesk::Asset {

void GenerateNormals(std::vector<Vertex>& vertices, const std::vector<std::uint32_t>& indices);

} // namespace DirectorDesk::Asset
