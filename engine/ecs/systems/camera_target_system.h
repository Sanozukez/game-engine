// engine/ECS/systems/camera_targe_system.h

#pragma once

#include "base_system.h"
#include "../../../client/camera/icamera.h"

// Forward Declarations
namespace Engine::ECS::Component {
    struct Transform;
    struct CameraTarget; 
}

namespace Engine {
namespace ECS {
namespace System {

class CameraTargetSystem : public BaseSystem {
private:
    Engine::Camera::ICamera &m_camera; // Referência injetada para setTarget()

public:
    CameraTargetSystem(Engine::Camera::ICamera &camera) : m_camera(camera) {}

    void update(World &world, float dt) override;
};

} // namespace System
} // namespace ECS
} // namespace Engine