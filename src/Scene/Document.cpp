// Document: Implementation for the DirectorDesk Scene module.
// This file owns project behavior only; keep platform and dependency boundaries explicit.

#include "DirectorDesk/Scene/Document.h"

#include <string>

namespace DirectorDesk::Scene {
namespace {

void NoteAllocatedId(std::uint32_t& nextIndex, const std::string& id) {
    if (id.size() > 5 && id.compare(0, 5, "node-") == 0) {
        try {
            const unsigned long value = std::stoul(id.substr(5));
            if (value >= nextIndex) {
                nextIndex = static_cast<std::uint32_t>(value + 1);
            }
        } catch (...) {
        }
    }
}

} // namespace

Node& Document::Add(Node node) {
    NoteAllocatedId(m_nextIndex, node.id);
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

bool Document::Remove(const std::string& id) {
    for (auto it = m_nodes.begin(); it != m_nodes.end(); ++it) {
        if (it->id == id) {
            m_nodes.erase(it);
            if (m_selectedId == id) {
                m_selectedId.clear();
            }
            return true;
        }
    }
    return false;
}

std::string Document::Duplicate(const std::string& id) {
    const Node* source = Find(id);
    if (source == nullptr) {
        return {};
    }
    Node copy = *source;
    copy.id = NextNodeId();
    copy.name = source->name + " 副本";
    copy.transform.position.x += 0.5f;
    Add(std::move(copy));
    return m_selectedId;
}

bool Document::SetVisible(const std::string& id, bool visible) {
    Node* node = Find(id);
    if (node == nullptr) {
        return false;
    }
    node->visible = visible;
    return true;
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
        NoteAllocatedId(m_nextIndex, node.id);
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
