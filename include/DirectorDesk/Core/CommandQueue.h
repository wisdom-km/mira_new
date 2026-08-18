// CommandQueue: Public or internal interface for the DirectorDesk Core module.
// This file owns project behavior only; keep platform and dependency boundaries explicit.

#pragma once

#include "DirectorDesk/Core/Command.h"

#include <deque>

namespace DirectorDesk::Core {

class CommandQueue {
public:
    void Push(Command command);
    bool TryPop(Command& out);
    [[nodiscard]] bool IsEmpty() const;

private:
    std::deque<Command> m_commands;
};

} // namespace DirectorDesk::Core
