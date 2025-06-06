// engine/game/player_character.h
#pragma once

#include "game_object.h" // Herda de GameObject
#include "./../../engine/input/input_manager.h" // Para Input::InputManager

namespace Engine {
namespace Game {

class PlayerCharacter : public GameObject {
public:
    PlayerCharacter();
    PlayerCharacter(std::unique_ptr<Engine::Asset::Model> model);
    virtual ~PlayerCharacter() = default; // Destrutor virtual

    // Sobrescrever o método update() do GameObject
    void update(float deltaTime, const Input::InputManager& inputManager, const Camera::ICamera& camera) override;

private:
    // Propriedades específicas do PlayerCharacter
    float m_movementSpeed;
    float m_rotationSpeed;

    // Para controlar a altura no terreno (futuro)
    // float m_targetHeight; 
};

} // namespace Game
} // namespace Engine