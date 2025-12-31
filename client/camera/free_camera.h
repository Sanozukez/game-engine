// engine/camera/free_camera.h
#pragma once

#include "icamera.h"
#include <glm/glm.hpp>

namespace Engine {
namespace Camera {

class FreeCamera final : public ICamera {
public:
    FreeCamera();

    // --- ICamera: Matrizes ---
    glm::mat4 getViewMatrix() const override;
    glm::mat4 getProjectionMatrix() const override;      // retorna por valor
    glm::mat4 getViewProjectionMatrix() const override;

    // --- ICamera: Consulta de estado ---
    float getZoom() const override;                      // FOV (deg)
    const glm::vec3& getPosition() const override;
    glm::vec3 getForwardVector() const override;
    glm::vec3 getRightVector() const override;
    float getYaw() const override;                       // deg
    float getPitch() const override;                     // deg
    glm::vec3 getTarget() const override;                // position + forward

    // --- ICamera: Mutação de estado ---
    void setPosition(const glm::vec3& position) override;
    void setTarget(const glm::vec3& target) override;    // reorienta para olhar o alvo
    void setZoom(float zoom_value) override;             // atualiza FOV e reprojeta
    void setYaw(float yaw_degrees) override;
    void setPitch(float pitch_degrees) override;

    // --- ICamera: Projeção ---
    void setProjectionMatrix(float fov_degrees,
                             float aspectRatio,
                             float nearPlane,
                             float farPlane) override;

    // --- ICamera: Input de usuário ---
    void processKeyboard(CameraMovement direction, float deltaTime) override;
    void processMouseMovement(double xpos, double ypos) override;
    void processScroll(double yOffset) override;         // ajusta FOV (zoom ótico)

    // --- Limites / Config externa (no-ops úteis) ---
    void setDistanceLimits(float minDistance, float maxDistance) override;
    void setPitchLimits(float minPitchDegrees, float maxPitchDegrees) override;
    void applyExternalConfig(const Engine::Core::ConfigManager& config) override;

private:
    // pose
    glm::vec3 m_position {0.0f, 1.0f, 5.0f};
    glm::vec3 m_front    {0.0f, 0.0f, -1.0f};
    glm::vec3 m_up       {0.0f, 1.0f,  0.0f};
    glm::vec3 m_right    {1.0f, 0.0f,  0.0f};
    glm::vec3 m_worldUp  {0.0f, 1.0f,  0.0f};

    // euler (deg)
    float m_yawDeg   {  0.0f};
    float m_pitchDeg {  0.0f};

    // controle
    float m_moveSpeed   {5.0f};
    float m_mouseSens   {0.1f};  // deg por pixel
    float m_fovDeg      {45.0f}; // FOV atual

    // projeção corrente
    float m_aspect {16.0f/9.0f};
    float m_near   {0.1f};
    float m_far    {1000.0f};
    glm::mat4 m_projection {1.0f};

    // estado de mouse livre (não-centrado)
    double m_lastMouseX {0.0};
    double m_lastMouseY {0.0};
    bool   m_firstMouse {true};

    // helpers
    void updateVectors();
    void rebuildProjection();     // usa m_fovDeg/m_aspect/m_near/m_far
};

} // namespace Camera
} // namespace Engine
