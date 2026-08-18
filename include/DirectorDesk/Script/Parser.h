// Parser: Public or internal interface for the DirectorDesk Script module.
// This file owns project behavior only; keep platform and dependency boundaries explicit.

#pragma once

#include "DirectorDesk/Script/Types.h"

#include <string>

namespace DirectorDesk::Script {

class Parser {
public:
    // Pure parser: malformed input produces diagnostics without mutating application state.
    static ParseResult Parse(const std::string& markdown);
};

} // namespace DirectorDesk::Script
