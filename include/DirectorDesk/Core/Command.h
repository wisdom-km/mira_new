#pragma once

#include <variant>

namespace DirectorDesk::Core {

struct QuitCommand {};

using Command = std::variant<QuitCommand>;

} // namespace DirectorDesk::Core
