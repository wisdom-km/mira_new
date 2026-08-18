// CameraManager: Public or internal interface for the DirectorDesk Camera module.
// This file owns project behavior only; keep platform and dependency boundaries explicit.

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
    std::string lastPreset;
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
    [[nodiscard]] CameraRig* Find(const std::string& id);
    [[nodiscard]] const CameraRig* Find(const std::string& id) const;
    void Replace(std::vector<CameraRig> cameras, std::string selectedId, LightPresetKind light);

private:
    std::string NextId();

    std::vector<CameraRig> m_cameras;
    std::string m_selectedId;
    LightPresetKind m_lightPreset = LightPresetKind::Neutral;
    std::uint32_t m_nextIndex = 1;
};

} // namespace DirectorDesk::Camera
