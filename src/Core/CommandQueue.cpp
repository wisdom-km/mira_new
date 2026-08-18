#include "DirectorDesk/Core/CommandQueue.h"

namespace DirectorDesk::Core {

void CommandQueue::Push(Command command) {
    m_commands.push_back(std::move(command));
}

bool CommandQueue::TryPop(Command& out) {
    if (m_commands.empty()) {
        return false;
    }
    out = std::move(m_commands.front());
    m_commands.pop_front();
    return true;
}

bool CommandQueue::IsEmpty() const {
    return m_commands.empty();
}

} // namespace DirectorDesk::Core
