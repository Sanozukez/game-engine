// engine/render/camera/orbit_camera.cpp
#include "orbit_camera.h"
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include "./../../../engine/core/log.h"
#include <glm/gtx/string_cast.hpp>

namespace Engine
{
    namespace Camera
    {

        OrbitCamera::OrbitCamera()
            : target_(0.0f, 0.0f, 0.0f),
              distance_(5.0f),
              pitch_(-0.5f), yaw_(0.7f),
              m_zoom(45.0f),
              m_lastMouseX(0.0), m_lastMouseY(0.0), m_firstMouse(true)
        {
            Engine::Log::Info("OrbitCamera: Construtor chamado.");
        }

        void OrbitCamera::setTarget(const glm::vec3 &target)
        {
            target_ = target;
            Engine::Log::Debug(std::format("OrbitCamera: Target definido para {}", glm::to_string(target_)));
        }

        void OrbitCamera::setDistance(float distance)
        {
            distance_ = glm::clamp(distance, 1.0f, 50.0f);
            Engine::Log::Debug(std::format("OrbitCamera: Distância definida para {}", distance_));
        }

        void OrbitCamera::setRotation(float pitch, float yaw)
        {
            pitch_ = pitch;
            yaw_ = yaw;
            Engine::Log::Debug(std::format("OrbitCamera: Rotação definida (pitch: {}, yaw: {}).", pitch_, yaw_));
        }

        void OrbitCamera::processMouseMovement(double xpos, double ypos)
        {
            if (m_firstMouse)
            {
                m_lastMouseX = xpos;
                m_lastMouseY = ypos;
                m_firstMouse = false;
                Engine::Log::Debug(std::format("OrbitCamera: First mouse movement handled. Initializing lastX: {}, lastY: {}", m_lastMouseX, m_lastMouseY));
                return;
            }

            float deltaX = static_cast<float>(xpos - m_lastMouseX);
            float deltaY = static_cast<float>(m_lastMouseY - ypos);

            m_lastMouseX = xpos;
            m_lastMouseY = ypos;

            const float sensitivity = 0.003f;

            yaw_ -= deltaX * sensitivity;
            pitch_ += deltaY * sensitivity;

            pitch_ = glm::clamp(pitch_, -0.8f, 0.8f);

            Engine::Log::Trace(std::format("OrbitCamera: Mouse moved (deltaX: {}, deltaY: {}). Pitch: {}, Yaw: {}.", deltaX, deltaY, pitch_, yaw_));
        }

        void OrbitCamera::processScroll(double yOffset)
        {
            setDistance(distance_ - static_cast<float>(yOffset));
            Engine::Log::Debug(std::format("OrbitCamera: Processed scroll. New distance: {}.", distance_));
        }

        void OrbitCamera::processKeyboard(CameraMovement direction, float deltaTime)
        {
            // OrbitCamera::processKeyboard não move mais o personagem ou gira seu yaw.
            // Apenas logs para depuração, pois o personagem é que processa o input.
            // Engine::Log::Trace(std::format("OrbitCamera: processKeyboard called for character input. Ignoring. Direction: {}.", static_cast<int>(direction)));
        }

        glm::mat4 OrbitCamera::getViewMatrix() const
        {
            float x = distance_ * cosf(pitch_) * sinf(yaw_);
            float y = distance_ * sinf(pitch_);
            float z = distance_ * cosf(pitch_) * cosf(yaw_);

            glm::vec3 cameraPos = target_ - glm::vec3(x, y, z);
            return glm::lookAt(cameraPos, target_, glm::vec3(0, 1, 0));
        }

        float OrbitCamera::getZoom() const
        { // REMOVIDO: override
            return m_zoom;
        }

        const glm::vec3 &OrbitCamera::getPosition() const
        { // REMOVIDO: override
            static glm::vec3 calculatedPos;
            float x = distance_ * cosf(pitch_) * sinf(yaw_);
            float y = distance_ * sinf(pitch_);
            float z = distance_ * cosf(pitch_) * cosf(yaw_);
            calculatedPos = target_ - glm::vec3(x, y, z);
            return calculatedPos;
        }

        // **** NOVO: Implementação de getYaw ****
        float OrbitCamera::getYaw() const
        { // REMOVIDO: override
            return yaw_;
        }

        glm::vec3 OrbitCamera::getForwardVector() const
        { // REMOVIDO: override
            return glm::normalize(glm::vec3(sin(glm::radians(yaw_)), 0.0f, cos(glm::radians(yaw_))));
        }

        glm::vec3 OrbitCamera::getRightVector() const
        { // REMOVIDO: override
            return glm::normalize(glm::cross(getForwardVector(), glm::vec3(0.0f, 1.0f, 0.0f)));
        }

        void OrbitCamera::setPosition(const glm::vec3 &position)
        { // REMOVIDO: override
            Engine::Log::Warn("OrbitCamera: setPosition called. Adjusting target based on new position.");
            target_ = position;
        }

        void OrbitCamera::resetMouseState()
        { // REMOVIDO: override
            m_firstMouse = true;
            Engine::Log::Debug("OrbitCamera: Mouse state reset (m_firstMouse = true).");
        }

        void OrbitCamera::setZoom(float zoom_value)
        { // REMOVIDO: override
            m_zoom = glm::clamp(zoom_value, 1.0f, 90.0f);
            Engine::Log::Debug(std::format("OrbitCamera: Zoom (FOV) definido para {}.", m_zoom));
        }

        void OrbitCamera::setYaw(float yaw_degrees)
        { // REMOVIDO: override
            yaw_ = yaw_degrees;
            Engine::Log::Trace(std::format("OrbitCamera: Yaw definido para {}.", yaw_degrees));
        }

    } // namespace Camera
} // namespace Engine