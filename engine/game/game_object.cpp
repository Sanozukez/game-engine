// engine/game/game_object.cpp
#include "game_object.h"
#include "./../core/log.h"
#include "./../asset/model.h"
#include "./../../engine/render/shader.h"
#include "./../../engine/input/input_manager.h"

#include <glm/gtx/quaternion.hpp>
#include <format>

#include <GLFW/glfw3.h>                           // Para códigos de tecla, se GameObject usar diretamente (apenas para PlayerCharacter agora)
#include "./../../engine/render/camera/icamera.h" // Para CameraMovement enum (apenas para PlayerCharacter agora)

namespace Engine
{
    namespace Game
    {

        GameObject::GameObject()
            : m_position(0.0f), m_rotation(glm::quat(1.0f, 0.0f, 0.0f, 0.0f)), m_scale(1.0f), m_model(nullptr), name("GameObject")
        {
            Engine::Log::Trace("GameObject: Construtor padrão chamado.");
        }

        GameObject::GameObject(std::unique_ptr<Engine::Asset::Model> model)
            : m_position(0.0f), m_rotation(glm::quat(1.0f, 0.0f, 0.0f, 0.0f)), m_scale(1.0f), m_model(std::move(model)), name("GameObject")
        {
            Engine::Log::Trace(std::format("GameObject: Construtor chamado com modelo."));
        }

        void GameObject::setPosition(const glm::vec3 &position)
        {
            m_position = position;
            Engine::Log::Trace(std::format("GameObject '{}': Posição definida para ({},{},{}).", name, position.x, position.y, position.z));
        }

        void GameObject::setRotation(const glm::quat &rotation)
        {
            m_rotation = rotation;
            Engine::Log::Trace(std::format("GameObject '{}': Rotação definida por quaternion.", name));
        }

        void GameObject::setRotationEuler(float pitch_deg, float yaw_deg, float roll_deg)
        {
            glm::vec3 euler_rad = glm::radians(glm::vec3(pitch_deg, yaw_deg, roll_deg));
            m_rotation = glm::quat(euler_rad);
            Engine::Log::Trace(std::format("GameObject '{}': Rotação definida por Euler (Pitch: {}, Yaw: {}, Roll: {}).", name, pitch_deg, yaw_deg, roll_deg));
        }

        float GameObject::getRotationYaw() const
        {
            return glm::degrees(glm::yaw(m_rotation));
        }

        void GameObject::setScale(const glm::vec3 &scale)
        {
            m_scale = scale;
            Engine::Log::Trace(std::format("GameObject '{}': Escala definida para ({},{},{}).", name, scale.x, scale.y, scale.z));
        }

        void GameObject::setScale(float scale)
        {
            m_scale = glm::vec3(scale);
            Engine::Log::Trace(std::format("GameObject '{}': Escala definida para {}.", name, scale));
        }

        glm::mat4 GameObject::getTransformMatrix() const
        {
            glm::mat4 modelMatrix = glm::mat4(1.0f);
            modelMatrix = glm::translate(modelMatrix, m_position);
            modelMatrix = modelMatrix * glm::mat4_cast(m_rotation);
            modelMatrix = glm::scale(modelMatrix, m_scale);
            return modelMatrix;
        }

        void GameObject::setModel(std::unique_ptr<Engine::Asset::Model> model)
        {
            m_model = std::move(model);
            Engine::Log::Trace(std::format("GameObject '{}': Modelo definido.", name));
        }

        void GameObject::draw(const Render::Shader &shader) const
        {
            if (m_model)
            {
                shader.setMat4("uModel", getTransformMatrix());
                m_model->draw(shader);
            }
            else
            {
                Engine::Log::Trace(std::format("GameObject '{}': Sem modelo para desenhar.", name));
            }
        }

        // **** MUDANÇA: Implementação padrão (vazia) do método update() ****
        void GameObject::update(float deltaTime, const Input::InputManager &inputManager, const Camera::ICamera &camera)
        { // MUDANÇA: Aceita camera
            // Default implementation, typically does nothing.
            // Derived classes will override this.
        }

    } // namespace Game
} // namespace Engine