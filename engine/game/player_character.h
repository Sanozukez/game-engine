// engine/game/player_character.h
#pragma once

#include "game_object.h"
#include "./../../engine/input/input_manager.h"
#include "./../../engine/render/camera/icamera.h"

// Forward Declaration
namespace Engine
{
    namespace Asset
    {
        class Model;
    }
    class Scene;
}

namespace Engine
{
    namespace Game
    {

        class PlayerCharacter : public GameObject
        {
        public:
            PlayerCharacter();
            PlayerCharacter(std::unique_ptr<Engine::Asset::Model> model);
            virtual ~PlayerCharacter() = default;

            void update(float deltaTime, const Input::InputManager &inputManager, Scene &scene, Camera::ICamera &camera) override;

            void moveTo(const glm::vec3 &destination);

            void setMovementSpeed(float speed);
            void setRotationSpeed(float degreesPerSecond);

            void setCameraFocusHeight(float height);
            glm::vec3 getCameraFocusPoint() const;

        private:
            float m_movementSpeed;
            float m_rotationSpeed;

            bool m_isMovingToDestination = false;
            glm::vec3 m_targetDestination{0.0f};

            // --- CORREÇÃO: Variável que faltava ser declarada ---
            bool m_isCameraOrbitModeActive = false;

            float m_cameraFocusHeight = 1.0f;
        };

    } // namespace Game
} // namespace Engine