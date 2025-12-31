// // engine/ecs/systems/camera_system.h
// Responsabilidade: Gerenciar a lógica de input e atualização das câmeras no World.

#pragma once

#include "base_system.h"
#include "../../../client/camera/icamera.h"
#include "../../../client/input/input_manager.h" // Dependência da sua lógica de input

namespace Engine {
namespace ECS {
namespace System {

class CameraSystem : public BaseSystem {
private:
    // O sistema precisa de uma referência direta à câmera (que hoje é um unique_ptr no App)
    Engine::Camera::ICamera& m_camera; 
    
    // O sistema usa o Singleton InputManager para obter o estado do teclado/mouse
    Engine::Input::InputManager& m_inputManager;

public:
    // Construtor: Aceita a referência da Câmera principal
    CameraSystem(Engine::Camera::ICamera& camera);
       
    void update(World& world, float dt) override;
    void processScrollCallback(const Engine::Input::InputEventData& data);
};

} // namespace System
} // namespace ECS
} // namespace Engine