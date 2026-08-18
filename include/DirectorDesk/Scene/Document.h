#pragma once

#include "DirectorDesk/Scene/Node.h"

#include <string>
#include <vector>

namespace DirectorDesk::Scene {

class Document {
public:
    Node& Add(Node node);
    [[nodiscard]] Node* Find(const std::string& id);
    [[nodiscard]] const Node* Find(const std::string& id) const;
    [[nodiscard]] const std::vector<Node>& Nodes() const {
        return m_nodes;
    }
    [[nodiscard]] bool IsEmpty() const {
        return m_nodes.empty();
    }

    void SetSelectedId(std::string id) {
        m_selectedId = std::move(id);
    }
    [[nodiscard]] const std::string& SelectedId() const {
        return m_selectedId;
    }
    [[nodiscard]] Node* Selected();
    [[nodiscard]] const Node* Selected() const;

    std::string NextNodeId();

private:
    std::vector<Node> m_nodes;
    std::string m_selectedId;
    std::uint32_t m_nextIndex = 1;
};

} // namespace DirectorDesk::Scene
