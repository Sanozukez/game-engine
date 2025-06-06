// engine/game/player_character.cpp
#define GLFW_INCLUDE_NONE
#include "player_character.h"
#include "./../core/log.h"
#include "./../asset/model.h"
#include "./../../engine/render/shader.h"
#include "./../../engine/input/input_manager.h"

#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/string_cast.hpp>
#include <format>

#include <GLFW/glfw3.h>
#include "./../../engine/render/camera/icamera.h"

namespace Engine
{
    namespace Game
    {

        PlayerCharacter::PlayerCharacter()
            : GameObject(), m_movementSpeed(5.0f), m_rotationSpeed(55.0f)
        {
            this->name = "PlayerCharacter";
            Engine::Log::Info("PlayerCharacter: Construtor padrão chamado.");
        }

        PlayerCharacter::PlayerCharacter(std::unique_ptr<Engine::Asset::Model> model)
            : GameObject(std::move(model)), m_movementSpeed(5.0f), m_rotationSpeed(55.0f)
        {
            this->name = "PlayerCharacter";
            Engine::Log::Info("PlayerCharacter: Construtor chamado com modelo.");
        }

        void PlayerCharacter::update(float deltaTime, const Input::InputManager &inputManager, const Camera::ICamera &camera)
        { // MUDANÇA: Aceita camera
            bool isRightMouseButtonPressed = inputManager.IsRightMouseButtonPressed();
            float currentY = m_position.y;

            // **** MUDANÇA CRUCIAL AQUI: Obter vetores de movimento da CAMERA ****
            glm::vec3 cameraForwardHorizontal = camera.getForwardVector();
            glm::vec3 cameraRightHorizontal = camera.getRightVector();

            float velocity = m_movementSpeed * deltaTime;
            float rotationSpeed = m_rotationSpeed * deltaTime;

            if (inputManager.IsKeyPressed(GLFW_KEY_W))
            {
                m_position += cameraForwardHorizontal * velocity;
            }
            if (inputManager.IsKeyPressed(GLFW_KEY_S))
            {
                m_position -= cameraForwardHorizontal * velocity;
            }

            if (isRightMouseButtonPressed)
            {
                // RMB held: A/Q movem para ESQUERDA; D/E movem para DIREITA (personagem strafa)
                if (inputManager.IsKeyPressed(GLFW_KEY_Q) || inputManager.IsKeyPressed(GLFW_KEY_A))
                {
                    m_position -= cameraRightHorizontal * velocity;
                }
                if (inputManager.IsKeyPressed(GLFW_KEY_E) || inputManager.IsKeyPressed(GLFW_KEY_D))
                {
                    m_position += cameraRightHorizontal * velocity;
                }
                // Com RMB, a rotação do mouse já gira a câmera, e o personagem deve seguir.
                // Sincronizar a rotação do personagem com a rotação horizontal da câmera
                // (Isso será feito em Scene::update, mas aqui é onde a intenção é mostrada)
                // m_rotation = glm::quat_cast(glm::lookAt(glm::vec3(0), cameraForwardHorizontal, glm::vec3(0,1,0))); // Exemplo
                // OR: m_rotation = glm::quat(glm::radians(glm::vec3(0.0f, camera.getYaw(), 0.0f))); // Se getYaw retorna yaw em graus
                // Vamos usar setRotationEuler para sincronizar.
                setRotationEuler(0.0f, camera.getYaw(), 0.0f); // Personagem rotaciona para onde a câmera está olhando horizontalmente
            }
            else
            {
                // RMB não segurado: Q/E movem (strafe); A/D rotacionam (personagem gira)
                if (inputManager.IsKeyPressed(GLFW_KEY_Q))
                {
                    m_position -= cameraRightHorizontal * velocity;
                }
                if (inputManager.IsKeyPressed(GLFW_KEY_E))
                {
                    m_position += cameraRightHorizontal * velocity;
                }
                if (inputManager.IsKeyPressed(GLFW_KEY_A))
                { // A rotaciona o personagem (m_rotation)
                    glm::quat yaw_rotation = glm::angleAxis(glm::radians(rotationSpeed), glm::vec3(0.0f, 1.0f, 0.0f));
                    m_rotation = yaw_rotation * m_rotation;
                }
                if (inputManager.IsKeyPressed(GLFW_KEY_D))
                { // D rotaciona o personagem
                    glm::quat yaw_rotation = glm::angleAxis(glm::radians(-rotationSpeed), glm::vec3(0.0f, 1.0f, 0.0f));
                    m_rotation = yaw_rotation * m_rotation;
                }
            }

            m_position.y = currentY;

            Engine::Log::Trace(std::format("PlayerCharacter '{}': Posição: {}, Rotação: {}.", name, glm::to_string(m_position), glm::to_string(glm::degrees(glm::eulerAngles(m_rotation)))));
        }

    } // namespace Game
} // namespace Engine