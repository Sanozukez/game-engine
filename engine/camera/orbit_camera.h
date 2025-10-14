// engine/camera/orbit_camera.h
#pragma once

#include "icamera.h"
#include <glm/glm.hpp>

namespace Engine {
namespace Camera {

class OrbitCamera final : public ICamera {
public:
    OrbitCamera();

    // --- ICamera: Matrizes ---
    glm::mat4 getViewMatrix() const override;
    glm::mat4 getProjectionMatrix() const override;      // retorna por valor
    glm::mat4 getViewProjectionMatrix() const override;

    // --- ICamera: Consulta de estado ---
    float getZoom() const override;
    const glm::vec3& getPosition() const override;       // calculado on-demand (armazenado em cache local)
    glm::vec3 getForwardVector() const override;
    glm::vec3 getRightVector() const override;
    float getYaw() const override;                        // graus
    float getPitch() const override;                      // graus
    glm::vec3 getTarget() const override;

    // --- ICamera: Mutação de estado ---
    void setPosition(const glm::vec3& position) override; // redefine como “target”
    void setTarget(const glm::vec3& target) override;
    void setZoom(float zoom_value) override;              // FOV em graus
    void setYaw(float yaw_degrees) override;              // graus
    void setPitch(float pitch_degrees) override;          // graus

    // --- ICamera: Projeção ---
    void setProjectionMatrix(float fov_degrees,
                             float aspectRatio,
                             float nearPlane,
                             float farPlane) override;

    // --- ICamera: Input de usuário (modo livre, não-centro) ---
    void processKeyboard(CameraMovement direction, float deltaTime) override;
    void processMouseMovement(double xpos, double ypos) override;
    void processScroll(double yOffset) override;

    // --- Limites / Config externa ---
    void setDistanceLimits(float minDistance, float maxDistance) override;
    void setPitchLimits(float minPitchDegrees, float maxPitchDegrees) override;
    void applyExternalConfig(const Engine::Core::ConfigManager& config) override;

    // --- Específicos da OrbitCamera ---
    void setDistance(float distance);                     // clamped nos limites
    void setRotation(float pitchRadians, float yawRadians); // utilitário interno (radianos)

private:
    // estado orbital
    glm::vec3 m_target {0.0f, 0.0f, 0.0f};
    float     m_distance {5.0f};      // raio
    float     m_pitchRad {-0.5f};     // rad
    float     m_yawRad {0.7f};        // rad
    float     m_fovDeg {45.0f};       // FOV

    // cache de posição calculada (mutável para retornar ref)
    mutable glm::vec3 m_cachedPos {0.0f, 0.0f, 0.0f};

    // estado de mouse “livre” (não centrado)
    double m_lastMouseX {0.0};
    double m_lastMouseY {0.0};
    bool   m_firstMouse {true};

    // projeção
    glm::mat4 m_projection {1.0f};

    // limites
    float m_minDistance {2.0f};
    float m_maxDistance {25.0f};
    float m_minPitchRad {glm::radians(-20.0f)};
    float m_maxPitchRad {glm::radians( 85.0f)};

    // helpers
    void clampPitch();
    void rebuildCachedPosition() const;
};

} // namespace Camera
} // namespace Engine
