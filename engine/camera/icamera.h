// engine/camera/icamera.h
#pragma once

#include <glm/glm.hpp>

// --- FORWARD DECLARATION CRÍTICA ---
namespace Engine
{
    namespace Core
    {
        class ConfigManager;
    }
}

namespace Engine
{
    namespace Camera
    {
        // Enum CameraMovement permanece o mesmo
        enum CameraMovement
        {
            FORWARD,
            BACKWARD,
            LEFT,
            RIGHT,
            UP,
            DOWN,
            ROTATE_LEFT,
            ROTATE_RIGHT
        };

        class ICamera
        {
        public:
            virtual ~ICamera() = default;

            // Métodos existentes
            virtual glm::mat4 getViewMatrix() const = 0;
            virtual float getZoom() const = 0;
            virtual const glm::vec3 &getPosition() const = 0;
            virtual glm::vec3 getForwardVector() const = 0;
            virtual glm::vec3 getRightVector() const = 0;
            virtual float getYaw() const = 0;
            virtual glm::vec3 getTarget() const = 0;
            virtual void processKeyboard(CameraMovement direction, float deltaTime) = 0;
            virtual void processMouseMovement(double xpos, double ypos) = 0;
            virtual void processScroll(double yOffset) = 0;

            virtual void setPosition(const glm::vec3 &position) = 0;
            virtual void setTarget(const glm::vec3 &target) = 0;
            virtual void resetMouseState() = 0;
            virtual void setZoom(float zoom_value) = 0;
            virtual void setYaw(float yaw_degrees) = 0;

            // --- NOVOS MÉTODOS VIRTUAIS ---
            // Define a matriz de projeção da câmera
            virtual void setProjectionMatrix(float fov_degrees, float aspectRatio, float nearPlane, float farPlane) = 0;
            // Retorna a matriz de projeção armazenada
            virtual const glm::mat4 &getProjectionMatrix() const = 0;

            virtual glm::mat4 getViewProjectionMatrix() const = 0;

            // NOVO: Usa o nome correto (Engine::Core::ConfigManager)
            virtual void setDistanceLimits(float minDistance, float maxDistance) = 0;
            virtual void setPitchLimits(float minPitchDegrees, float maxPitchDegrees) = 0;

            // CRÍTICO: Assinatura correta para a interface
            virtual void applyExternalConfig(const Engine::Core::ConfigManager &config) = 0;
        };

    } // namespace Camera
} // namespace Engine