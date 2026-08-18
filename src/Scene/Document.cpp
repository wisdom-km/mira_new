#include "DirectorDesk/Scene/Document.h"

#include <string>

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

void Document::Clear() {
    m_nodes.clear();
    m_selectedId.clear();
    m_nextIndex = 1;
}

void Document::ReplaceNodes(std::vector<Node> nodes, std::string selectedId) {
    m_nodes = std::move(nodes);
    m_nextIndex = 1;
    for (const Node& node : m_nodes) {
        if (node.id.size() > 5 && node.id.compare(0, 5, "node-") == 0) {
            try {
                const unsigned long value = std::stoul(node.id.substr(5));
                if (value >= m_nextIndex) {
                    m_nextIndex = static_cast<std::uint32_t>(value + 1);
                }
            } catch (...) {
            }
        }
    }
    if (Find(selectedId) != nullptr) {
        m_selectedId = std::move(selectedId);
    } else if (!m_nodes.empty()) {
        m_selectedId = m_nodes.front().id;
    } else {
        m_selectedId.clear();
    }
}

} // namespace DirectorDesk::Scene
