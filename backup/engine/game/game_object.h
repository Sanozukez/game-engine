// engine/game/game_object.h
#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <memory>
#include <string>

// Forward declarations
namespace Engine
{
    namespace Asset
    {
        class Model;
    }
    namespace Render
    {
        class Shader;
    }
    namespace Input
    {
        class InputManager;
        enum class CameraMovement;
    }
    namespace Camera
    {
        class ICamera;
    }
    class Scene;
} // namespace Engine

namespace Engine
{
    namespace Game
    {

        class GameObject
        {
        public:
            GameObject();
            GameObject(std::unique_ptr<Engine::Asset::Model> model);
            virtual ~GameObject() = default;

            void setPosition(const glm::vec3 &position);
            void setRotation(const glm::quat &rotation);
            void setRotationEuler(float pitch_deg, float yaw_deg, float roll_deg);
            void setScale(const glm::vec3 &scale);
            void setScale(float scale);

            const glm::vec3 &getPosition() const { return m_position; }
            const glm::quat &getRotation() const { return m_rotation; }
            float getRotationYaw() const;
            const glm::vec3 &getScale() const { return m_scale; }

            glm::mat4 getTransformMatrix() const;

            void setModel(std::unique_ptr<Engine::Asset::Model> model);
            Engine::Asset::Model *getModel() const { return m_model.get(); }

            void draw(Engine::Render::Shader &shader);

            // --- MUDANÇA AQUI ---
            // A função virtual 'update' agora aceita uma referência NÃO-constante para a câmera,
            // permitindo que classes filhas (como PlayerCharacter) possam modificá-la.
            virtual void update(float deltaTime, const Input::InputManager &inputManager, Scene& scene, Camera::ICamera &camera);

            std::string name;

        protected:
            glm::vec3 m_position;
            glm::quat m_rotation;
            glm::vec3 m_scale;

            std::unique_ptr<Engine::Asset::Model> m_model;
        };

    } // namespace Game
} // namespace Engine