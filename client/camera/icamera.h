// engine/camera/icamera.h
#pragma once

#include <glm/glm.hpp>

// --- Forward declaration para evitar dependência pesada de headers ---
namespace Engine {
namespace Core {
    class ConfigManager;
}
}

namespace Engine {
namespace Camera {

    // Mantém os mesmos movimentos básicos
    enum CameraMovement {
        FORWARD,
        BACKWARD,
        LEFT,
        RIGHT,
        UP,
        DOWN,
        ROTATE_LEFT,
        ROTATE_RIGHT
    };

    // Interface de câmera minimalista e consistente.
    // Responsabilidades:
    // - View/Projection
    // - Posição/orientação (yaw/pitch)
    // - Movimento por teclado/mouse/scroll
    // - Alvos/limites/config externa
    class ICamera {
    public:
        virtual ~ICamera() = default;

        // --- Matrizes ---
        virtual glm::mat4 getViewMatrix() const = 0;
        virtual glm::mat4 getProjectionMatrix() const = 0;
        virtual glm::mat4 getViewProjectionMatrix() const = 0;

        // --- Consulta de estado ---
        virtual float getZoom() const = 0;
        virtual const glm::vec3& getPosition() const = 0;
        virtual glm::vec3 getForwardVector() const = 0;
        virtual glm::vec3 getRightVector() const = 0;
        virtual float getYaw() const = 0;
        virtual float getPitch() const = 0;
        virtual glm::vec3 getTarget() const = 0;

        // --- Mutação de estado ---
        virtual void setPosition(const glm::vec3& position) = 0;
        virtual void setTarget(const glm::vec3& target) = 0;
        virtual void setZoom(float zoom_value) = 0;
        virtual void setYaw(float yaw_degrees) = 0;
        virtual void setPitch(float pitch_degrees) = 0;

        // --- Projeção ---
        virtual void setProjectionMatrix(float fov_degrees,
                                         float aspectRatio,
                                         float nearPlane,
                                         float farPlane) = 0;

        // --- Input de usuário ---
        virtual void processKeyboard(CameraMovement direction, float deltaTime) = 0;
        virtual void processMouseMovement(double xpos, double ypos) = 0; // útil para modos não “travados no centro”
        virtual void processScroll(double yOffset) = 0;

        // --- Limites / Config externa ---
        virtual void setDistanceLimits(float minDistance, float maxDistance) = 0;
        virtual void setPitchLimits(float minPitchDegrees, float maxPitchDegrees) = 0;
        virtual void applyExternalConfig(const Engine::Core::ConfigManager& config) = 0;
    };

} // namespace Camera
} // namespace Engine
