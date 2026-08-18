// Document: Public or internal interface for the DirectorDesk Scene module.
// This file owns project behavior only; keep platform and dependency boundaries explicit.

#pragma once

#include "DirectorDesk/Scene/Node.h"

#include <string>
#include <vector>

namespace DirectorDesk::Scene {

class Document {
public:
    // Owns stable scene-node IDs and the CPU-side source of truth for rendering.
    Node& Add(Node node);
    // Returns null instead of throwing when a persisted or UI-provided ID is absent.
    [[nodiscard]] Node* Find(const std::string& id);
    [[nodiscard]] const Node* Find(const std::string& id) const;
    [[nodiscard]] const std::vector<Node>& Nodes() const {
        return m_nodes;
    }
    [[nodiscard]] bool IsEmpty() const {
        return m_nodes.empty();
    }

    // Selection is stored by stable ID so vector reordering cannot invalidate it.
    void SetSelectedId(std::string id) {
        m_selectedId = std::move(id);
    }
    [[nodiscard]] const std::string& SelectedId() const {
        return m_selectedId;
    }
    [[nodiscard]] Node* Selected();
    [[nodiscard]] const Node* Selected() const;

    std::string NextNodeId();
    void Clear();
    // Replaces a loaded project snapshot atomically from the caller's perspective.
    void ReplaceNodes(std::vector<Node> nodes, std::string selectedId);

private:
    std::vector<Node> m_nodes;
    std::string m_selectedId;
    std::uint32_t m_nextIndex = 1;
};

} // namespace DirectorDesk::Scene
