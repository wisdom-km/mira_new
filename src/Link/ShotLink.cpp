// ShotLink: Implementation for the DirectorDesk Link module.
// This file owns project behavior only; keep platform and dependency boundaries explicit.

#include "DirectorDesk/Link/ShotLink.h"

namespace DirectorDesk::Link {

void Table::Set(std::string shotId, std::string cameraId) {
    if (shotId.empty() || cameraId.empty()) {
        return;
    }
    for (ShotLink& link : m_links) {
        if (link.shotId == shotId) {
            link.cameraId = std::move(cameraId);
            return;
        }
    }
    m_links.push_back(ShotLink{std::move(shotId), std::move(cameraId)});
}

void Table::ClearShot(const std::string& shotId) {
    for (auto it = m_links.begin(); it != m_links.end(); ++it) {
        if (it->shotId == shotId) {
            m_links.erase(it);
            return;
        }
    }
}

void Table::Clear() {
    m_links.clear();
}

const std::string* Table::CameraForShot(const std::string& shotId) const {
    for (const ShotLink& link : m_links) {
        if (link.shotId == shotId) {
            return &link.cameraId;
        }
    }
    return nullptr;
}

void Table::Replace(std::vector<ShotLink> links) {
    m_links = std::move(links);
}

} // namespace DirectorDesk::Link
