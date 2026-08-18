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
