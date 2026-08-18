#include "DirectorDesk/Camera/CameraManager.h"

namespace DirectorDesk::Camera {

CameraManager::CameraManager() {
    Add("Camera 1");
}

std::string CameraManager::NextId() {
    return "cam-" + std::to_string(m_nextIndex++);
}

CameraRig* CameraManager::Find(const std::string& id) {
    for (CameraRig& camera : m_cameras) {
        if (camera.id == id) {
            return &camera;
        }
    }
    return nullptr;
}

const CameraRig* CameraManager::Find(const std::string& id) const {
    for (const CameraRig& camera : m_cameras) {
        if (camera.id == id) {
            return &camera;
        }
    }
    return nullptr;
}

CameraRig* CameraManager::Selected() {
    return Find(m_selectedId);
}

const CameraRig* CameraManager::Selected() const {
    return Find(m_selectedId);
}

CameraRig& CameraManager::Add(std::string name) {
    CameraRig rig;
    rig.id = NextId();
    rig.name = name.empty() ? "Camera " + std::to_string(m_cameras.size() + 1) : std::move(name);
    m_cameras.push_back(std::move(rig));
    m_selectedId = m_cameras.back().id;
    return m_cameras.back();
}

bool CameraManager::Remove(const std::string& id) {
    if (m_cameras.size() <= 1) {
        return false;
    }
    for (auto it = m_cameras.begin(); it != m_cameras.end(); ++it) {
        if (it->id != id) {
            continue;
        }
        const bool removingSelected = m_selectedId == id;
        m_cameras.erase(it);
        if (removingSelected) {
            m_selectedId = m_cameras.front().id;
        }
        return true;
    }
    return false;
}

bool CameraManager::Rename(const std::string& id, std::string name) {
    CameraRig* camera = Find(id);
    if (camera == nullptr) {
        return false;
    }
    camera->name = name.empty() ? "未命名" : std::move(name);
    return true;
}

bool CameraManager::Select(const std::string& id) {
    if (Find(id) == nullptr) {
        return false;
    }
    m_selectedId = id;
    return true;
}

void CameraManager::ApplyPreset(CameraPresetKind kind, const SubjectFrame& subject) {
    CameraRig* camera = Selected();
    if (camera == nullptr) {
        return;
    }
    const CameraPose pose = ResolveCameraPreset(kind, subject);
    camera->orbit.ApplyPose(pose);
}

void CameraManager::SetLightPreset(LightPresetKind kind) {
    m_lightPreset = kind;
}

LightState CameraManager::CurrentLight() const {
    return ResolveLightPreset(m_lightPreset);
}

} // namespace DirectorDesk::Camera
