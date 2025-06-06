// engine/render/camera/free_camera.h

#ifndef FREE_CAMERA_H
#define FREE_CAMERA_H

#include "icamera.h" 
#include <glm/glm.hpp>

namespace Engine { 
namespace Camera { 

class FreeCamera : public ICamera { 
public:
    FreeCamera();

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
    // **** NOVO: getYaw (Declarado corretamente aqui) ****
    float getYaw() const override; 


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

    double m_lastMouseX = 0.0;
    double m_lastMouseY = 0.0;
    bool m_firstMouse = true;

    void updateVectors();
};

} // namespace Camera
} // namespace Engine

#endif