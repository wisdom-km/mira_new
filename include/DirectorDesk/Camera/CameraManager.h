#pragma once

#include "DirectorDesk/Camera/OrbitCamera.h"
#include "DirectorDesk/Camera/Presets.h"

#include <cstdint>
#include <string>
#include <vector>

namespace DirectorDesk::Camera {

struct CameraRig {
    std::string id;
    std::string name;
    OrbitCamera orbit;
};

class CameraManager {
public:
    CameraManager();

    CameraRig& Add(std::string name = {});
    bool Remove(const std::string& id);
    bool Rename(const std::string& id, std::string name);
    bool Select(const std::string& id);
    void ApplyPreset(CameraPresetKind kind, const SubjectFrame& subject);
    void SetLightPreset(LightPresetKind kind);

    [[nodiscard]] const std::vector<CameraRig>& Cameras() const {
        return m_cameras;
    }
    [[nodiscard]] const std::string& SelectedId() const {
        return m_selectedId;
    }
    [[nodiscard]] CameraRig* Selected();
    [[nodiscard]] const CameraRig* Selected() const;
    [[nodiscard]] LightPresetKind LightPreset() const {
        return m_lightPreset;
    }
    [[nodiscard]] LightState CurrentLight() const;

private:
    CameraRig* Find(const std::string& id);
    const CameraRig* Find(const std::string& id) const;
    std::string NextId();

    std::vector<CameraRig> m_cameras;
    std::string m_selectedId;
    LightPresetKind m_lightPreset = LightPresetKind::Neutral;
    std::uint32_t m_nextIndex = 1;
};

} // namespace DirectorDesk::Camera
