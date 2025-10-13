// engine/camera/free_camera.h
#pragma once // Substituído o #ifndef guard por #pragma once para consistência

#include "icamera.h" 
#include <glm/glm.hpp>

namespace Engine { 
namespace Camera { 

class FreeCamera : public ICamera { 
public:
    FreeCamera();

    // Métodos existentes (sobrescritos)
    void setPosition(const glm::vec3& position) override;
    glm::mat4 getViewMatrix() const override;
    void processKeyboard(CameraMovement direction, float deltaTime) override;
    void processMouseMovement(double xpos, double ypos) override; 
    void processScroll(double yOffset) override; 
    float getZoom() const override; 
    const glm::vec3& getPosition() const override; 
    glm::vec3 getForwardVector() const override; 
    glm::vec3 getRightVector() const override;
    void resetMouseState() override; 
    void setTarget(const glm::vec3& target) override; 
    void setZoom(float zoom_value) override; 
    void setYaw(float yaw_degrees) override;
    float getYaw() const override;
    glm::vec3 getTarget() const override; 

    // --- NOVOS MÉTODOS DA INTERFACE IMPLEMENTADOS ---
    void setProjectionMatrix(float fov_degrees, float aspectRatio, float nearPlane, float farPlane) override;
    const glm::mat4& getProjectionMatrix() const override;

    glm::mat4 getViewProjectionMatrix() const override;

private:
    glm::vec3 position;
    glm::vec3 front;
    glm::vec3 up;
    glm::vec3 right;
    glm::vec3 worldUp;

    float yaw; 
    float pitch;
    float speed;
    float sensitivity;
    float zoom; 

    double m_lastMouseX;
    double m_lastMouseY;
    bool m_firstMouse;

    // --- NOVO MEMBRO ADICIONADO ---
    glm::mat4 m_projectionMatrix;

    void updateVectors();
};

} // namespace Camera
} // namespace Engine