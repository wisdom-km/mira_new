#pragma once

#include "DirectorDesk/UI/IPanel.h"

#include <cstdint>
#include <string>

namespace DirectorDesk::UI {

class ScriptPanel final : public IPanel {
public:
    void Draw(const AppViewState& state, Core::CommandQueue& commands) override;

private:
    std::string m_editorText;
    std::uint64_t m_seenRevision = 0;
};

} // namespace DirectorDesk::UI
