// engine/render/camera/free_camera.cpp

#include "free_camera.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/string_cast.hpp> 
#include "./../../../engine/core/log.h" 

namespace Engine { 
namespace Camera {

FreeCamera::FreeCamera()
    : position(glm::vec3(0.0f, 1.0f, 5.0f)),
      worldUp(glm::vec3(0.0f, 1.0f, 0.0f)),
      yaw(0.0f), 
      pitch(0.0f), speed(5.0f), sensitivity(0.1f), 
      zoom(45.0f) { 
    updateVectors();
}

void FreeCamera::setPosition(const glm::vec3& position) { 
    this->position = position;
    Engine::Log::Debug(std::format("FreeCamera: Posição definida para {}", glm::to_string(position)));
}

const glm::vec3& FreeCamera::getPosition() const { 
    return position;
}

float FreeCamera::getZoom() const { 
    return zoom;
}

float FreeCamera::getYaw() const { 
    return yaw;
}

void FreeCamera::updateVectors() {
    glm::vec3 newFront;
    newFront.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    newFront.y = sin(glm::radians(pitch));
    newFront.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    front = glm::normalize(newFront);

    right = glm::normalize(glm::cross(front, worldUp));
    up = glm::normalize(glm::cross(right, front));
}

void FreeCamera::processMouseMovement(double xpos, double ypos) { 
    if (m_firstMouse) {
        m_lastMouseX = xpos;
        m_lastMouseY = ypos;
        m_firstMouse = false;
        Engine::Log::Debug(std::format("FreeCamera: First mouse movement handled. Initializing lastX: {}, lastY: {}", m_lastMouseX, m_lastMouseY));
        return; 
    }

    float deltaX = static_cast<float>(xpos - m_lastMouseX);
    float deltaY = static_cast<float>(m_lastMouseY - ypos); 

    m_lastMouseX = xpos;
    m_lastMouseY = ypos;

    deltaX *= sensitivity;
    deltaY *= sensitivity;

    yaw += deltaX;
    pitch += deltaY;

    if (pitch > 89.0f)
        pitch = 89.0f;
    if (pitch < -89.0f)
        pitch = -89.0f;

    updateVectors();
}

void FreeCamera::processKeyboard(CameraMovement direction, float deltaTime) { 
    float velocity = speed * deltaTime;

    switch (direction) {
        case FORWARD:
            position += front * velocity;
            break;
        case BACKWARD:
            position -= front * velocity;
            break;
        case LEFT:
            position -= right * velocity;
            break;
        case RIGHT:
            position += right * velocity;
            break;
        case UP:
            position += worldUp * velocity;
            break;
        case DOWN:
            position -= worldUp * velocity;
            break;
        case ROTATE_LEFT: 
            Engine::Log::Trace("FreeCamera: Ignoring ROTATE_LEFT keyboard input.");
            break;
        case ROTATE_RIGHT: 
            Engine::Log::Trace("FreeCamera: Ignoring ROTATE_RIGHT keyboard input.");
            break;
    }
}

glm::mat4 FreeCamera::getViewMatrix() const { 
    return glm::lookAt(position, position + front, up);
}

void FreeCamera::resetMouseState() { 
    m_firstMouse = true;
    Engine::Log::Debug("FreeCamera: Mouse state reset (m_firstMouse = true).");
}

void FreeCamera::processScroll(double yOffset) { 
    // FreeCamera não usa scroll para zoom, então deixamos vazio.
}

void FreeCamera::setTarget(const glm::vec3& target) { 
    Engine::Log::Warn("FreeCamera: setTarget called, but control is via position. Ignoring or adjusting.");
}

void FreeCamera::setZoom(float zoom_value) { 
    zoom = glm::clamp(zoom_value, 1.0f, 90.0f);
    Engine::Log::Debug(std::format("FreeCamera: Zoom (FOV) definido para {}.", zoom));
}

void FreeCamera::setYaw(float yaw_degrees) { 
    yaw = yaw_degrees;
    updateVectors(); 
    Engine::Log::Trace(std::format("FreeCamera: Yaw definido para {}.", yaw_degrees));
}

// **** NOVOS: Implementação de getForwardVector e getRightVector ****
glm::vec3 FreeCamera::getForwardVector() const { 
    return glm::normalize(glm::vec3(front.x, 0.0f, front.z)); 
}

glm::vec3 FreeCamera::getRightVector() const { 
    return glm::normalize(glm::cross(getForwardVector(), glm::vec3(0.0f, 1.0f, 0.0f)));
}

} // namespace Camera
} // namespace Engine