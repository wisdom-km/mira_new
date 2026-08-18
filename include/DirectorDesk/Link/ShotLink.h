#pragma once

#include <string>
#include <vector>

namespace DirectorDesk::Link {

struct ShotLink {
    std::string shotId;
    std::string cameraId;
};

class Table {
public:
    void Set(std::string shotId, std::string cameraId);
    void ClearShot(const std::string& shotId);
    void Clear();
    [[nodiscard]] const std::string* CameraForShot(const std::string& shotId) const;
    [[nodiscard]] const std::vector<ShotLink>& All() const {
        return m_links;
    }
    void Replace(std::vector<ShotLink> links);

private:
    std::vector<ShotLink> m_links;
};

} // namespace DirectorDesk::Link
