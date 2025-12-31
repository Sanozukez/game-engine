// engine/camera/free_camera.cpp
#include "free_camera.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtx/compatibility.hpp>   // glm::clamp para floats
#include <cmath>

namespace Engine {
namespace Camera {

// ---- Helpers internos ----
static inline float rad2deg(float r) { return r * (180.0f / glm::pi<float>()); }
static inline float deg2rad(float d) { return d * (glm::pi<float>() / 180.0f); }

// ---------------------------------
// Ctor
// ---------------------------------
FreeCamera::FreeCamera() {
    rebuildProjection();
    updateVectors();
}

// ---------------------------------
// ICamera: Matrizes
// ---------------------------------
glm::mat4 FreeCamera::getViewMatrix() const {
    return glm::lookAt(m_position, m_position + m_front, m_up);
}

glm::mat4 FreeCamera::getProjectionMatrix() const {
    return m_projection; // retorno por valor, como no .h
}

glm::mat4 FreeCamera::getViewProjectionMatrix() const {
    return m_projection * getViewMatrix();
}

// ---------------------------------
// ICamera: Consulta de estado
// ---------------------------------
float FreeCamera::getZoom() const {
    return m_fovDeg; // tratamos "zoom" como FOV (graus)
}

const glm::vec3& FreeCamera::getPosition() const {
    return m_position;
}

glm::vec3 FreeCamera::getForwardVector() const {
    return glm::normalize(m_front);
}

glm::vec3 FreeCamera::getRightVector() const {
    return glm::normalize(m_right);
}

float FreeCamera::getYaw() const {
    return m_yawDeg;
}

float FreeCamera::getPitch() const {
    return m_pitchDeg;
}

glm::vec3 FreeCamera::getTarget() const {
    return m_position + m_front;
}

// ---------------------------------
// ICamera: Mutação de estado
// ---------------------------------
void FreeCamera::setPosition(const glm::vec3& position) {
    m_position = position;
}

void FreeCamera::setTarget(const glm::vec3& target) {
    // Reorienta para olhar o alvo, preservando a posição
    glm::vec3 dir = glm::normalize(target - m_position);
    // Yaw: ângulo no plano XZ, medido a partir do +Z (para ser consistente com updateVectors)
    // Pitch: inclinação para cima/baixo
    m_pitchDeg = rad2deg(std::asin(glm::clamp(dir.y, -1.0f, 1.0f)));
    // atan2(sinYaw, cosYaw) conforme a convenção usada em updateVectors (z leva o cos, x leva o sin)
    m_yawDeg = rad2deg(std::atan2(dir.z, dir.x)) - 90.0f; // equivalência com a fórmula de updateVectors
    // Ajusta vetores
    updateVectors();
}

void FreeCamera::setZoom(float zoom_value) {
    m_fovDeg = glm::clamp(zoom_value, 1.0f, 90.0f);
    rebuildProjection();
}

void FreeCamera::setYaw(float yaw_degrees) {
    m_yawDeg = yaw_degrees;
    updateVectors();
}

void FreeCamera::setPitch(float pitch_degrees) {
    m_pitchDeg = glm::clamp(pitch_degrees, -89.0f, 89.0f);
    updateVectors();
}

// ---------------------------------
// ICamera: Projeção
// ---------------------------------
void FreeCamera::setProjectionMatrix(float fov_degrees,
                                     float aspectRatio,
                                     float nearPlane,
                                     float farPlane) {
    m_fovDeg = fov_degrees;
    m_aspect = aspectRatio;
    m_near   = nearPlane;
    m_far    = farPlane;
    rebuildProjection();
}

// ---------------------------------
// ICamera: Input
// ---------------------------------
void FreeCamera::processKeyboard(CameraMovement direction, float dt) {
    const float vel = m_moveSpeed * dt;

    switch (direction) {
        case FORWARD:  m_position += m_front * vel;           break;
        case BACKWARD: m_position -= m_front * vel;           break;
        case LEFT:     m_position -= m_right * vel;           break;
        case RIGHT:    m_position += m_right * vel;           break;
        case UP:       m_position += m_worldUp * vel;         break;
        case DOWN:     m_position -= m_worldUp * vel;         break;
        case ROTATE_LEFT:
        case ROTATE_RIGHT:
            // FreeCamera não usa esses enums para girar por teclado; ignore.
            break;
    }
}

void FreeCamera::processMouseMovement(double xpos, double ypos) {
    if (m_firstMouse) {
        m_lastMouseX = xpos;
        m_lastMouseY = ypos;
        m_firstMouse = false;
        return; // debounce do primeiro delta
    }

    float dx = static_cast<float>(xpos - m_lastMouseX);
    float dy = static_cast<float>(m_lastMouseY - ypos); // invertido

    m_lastMouseX = xpos;
    m_lastMouseY = ypos;

    m_yawDeg   += dx * m_mouseSens;   // graus por pixel
    m_pitchDeg += dy * m_mouseSens;

    m_pitchDeg = glm::clamp(m_pitchDeg, -89.0f, 89.0f);

    updateVectors();
}

void FreeCamera::processScroll(double yOffset) {
    // Aproxima/afasta ajustando FOV (zoom ótico)
    m_fovDeg = glm::clamp(m_fovDeg - static_cast<float>(yOffset), 1.0f, 90.0f);
    rebuildProjection();
}

// ---------------------------------
// Limites / Config externa
// ---------------------------------
void FreeCamera::setDistanceLimits(float, float) {
    // Não aplicável em free camera (sem pivô/órbita)
}

void FreeCamera::setPitchLimits(float, float) {
    // Sem limites custom — já clampamos em [-89, 89]
}

void FreeCamera::applyExternalConfig(const Engine::Core::ConfigManager&) {
    // No-op elegante; pode-se ler velocidade, sensibilidade, FOV, etc. no futuro.
}

// ---------------------------------
// Helpers
// ---------------------------------
void FreeCamera::updateVectors() {
    // Constrói front a partir de yaw/pitch (graus) com a mesma convenção em todo o engine
    const float yawRad   = deg2rad(m_yawDeg);
    const float pitchRad = deg2rad(m_pitchDeg);

    glm::vec3 front;
    front.x = std::cos(pitchRad) * std::cos(yawRad);
    front.y = std::sin(pitchRad);
    front.z = std::cos(pitchRad) * std::sin(yawRad);
    m_front = glm::normalize(front);

    m_right = glm::normalize(glm::cross(m_front, m_worldUp));
    m_up    = glm::normalize(glm::cross(m_right, m_front));
}

void FreeCamera::rebuildProjection() {
    m_projection = glm::perspective(glm::radians(m_fovDeg), m_aspect, m_near, m_far);
}

} // namespace Camera
} // namespace Engine
