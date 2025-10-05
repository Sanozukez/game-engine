// src/app/scene.h
#pragma once

#include <memory> 
#include <vector>
#include <glm/glm.hpp> 
#include "./../../engine/core/SceneLoader.h"
#include "./../../shared/mmap_format/SceneFileFormat.h"
#include "./../../engine/render/camera/icamera.h" 
#include "./../../engine/render/texture.h"

// Forward Declaration para o WorldInitializer
namespace Engine::Game {
    class WorldInitializer; 
}

namespace Engine {
    namespace Render {
        class Shader;
        class Material; 
    }
    namespace Asset {
        class Model; 
    }
    namespace Game {
        class GameObject;
        class PlayerCharacter;
    }
    namespace Input { 
        class InputManager; 
    }
}

#include "./../../engine/render/camera/free_camera.h"
#include "./../../engine/render/camera/orbit_camera.h"

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

    void addGameObject(std::unique_ptr<Engine::Game::GameObject> obj);
    
    // Método que adicionamos para acessar o player a partir do App
    Engine::Game::PlayerCharacter* getPlayer();

    Engine::Game::GameObject* getTerrain();

private:
    std::unique_ptr<Engine::Camera::ICamera> m_camera; 
    std::unique_ptr<Engine::Render::Shader> shader; 
    
    std::vector<std::unique_ptr<Engine::Game::GameObject>> m_gameObjects; 
    Engine::Game::PlayerCharacter* m_playerCharacter = nullptr;

    Engine::Game::GameObject* m_terrain = nullptr;
};

} // namespace Engine