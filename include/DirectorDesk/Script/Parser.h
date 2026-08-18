#pragma once

#include "DirectorDesk/Script/Types.h"

#include <string>

namespace DirectorDesk::Script {

class Parser {
public:
    static ParseResult Parse(const std::string& markdown);
};

} // namespace DirectorDesk::Script
