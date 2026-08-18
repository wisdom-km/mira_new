#include "DirectorDesk/Scene/Document.h"

namespace DirectorDesk::Scene {

Node& Document::Add(Node node) {
    m_nodes.push_back(std::move(node));
    m_selectedId = m_nodes.back().id;
    return m_nodes.back();
}

Node* Document::Find(const std::string& id) {
    for (Node& node : m_nodes) {
        if (node.id == id) {
            return &node;
        }
    }
    return nullptr;
}

const Node* Document::Find(const std::string& id) const {
    for (const Node& node : m_nodes) {
        if (node.id == id) {
            return &node;
        }
    }
    return nullptr;
}

Node* Document::Selected() {
    return Find(m_selectedId);
}

const Node* Document::Selected() const {
    return Find(m_selectedId);
}

std::string Document::NextNodeId() {
    return "node-" + std::to_string(m_nextIndex++);
}

} // namespace DirectorDesk::Scene
