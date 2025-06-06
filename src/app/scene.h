// src/app/scene.h
#pragma once

#include <memory> 
#include <glm/glm.hpp> 
#include "./../../engine/render/camera/icamera.h" 
#include "./../../engine/render/texture.h" 

// Forward declarations para as classes necessárias
namespace Engine {
namespace Geometry {
    // ...
}
namespace Render {
    class Shader;
    class Material; 
}
namespace Asset {
    class Model; 
}
namespace Game {
    class GameObject;
    class PlayerCharacter; // NOVO: Forward declaration para PlayerCharacter
}
namespace Input { 
    class InputManager; 
}
} // namespace Engine

#include "./../../engine/render/camera/free_camera.h"
#include "./../../engine/render/camera/orbit_camera.h"
#include "./../../engine/core/config.h"


namespace Engine { 

class Scene { 
public:
    Scene();
    ~Scene(); 

    void initialize();
    void update(float deltaTime, const Input::InputManager& inputManager); 
    void render(const glm::mat4& projection, const glm::mat4& view) const; 
    
    Engine::Camera::ICamera& getCamera();
    void setCamera(std::unique_ptr<Engine::Camera::ICamera> camera);

private:
    std::unique_ptr<Engine::Camera::ICamera> m_camera; 
    
    std::unique_ptr<Engine::Render::Shader> shader; 
    
    // Gerenciar GameObjects
    std::vector<std::unique_ptr<Engine::Game::GameObject>> m_gameObjects; 
    // **** MUDANÇA AQUI: Ponteiro para o PlayerCharacter ****
    Engine::Game::PlayerCharacter* m_playerCharacter = nullptr; // NOVO: Ponteiro para o GameObject do personagem (PlayerCharacter)
};

} // namespace Engine