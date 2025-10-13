// // engine/camera/orbit_camera.cpp (VERSÃO FINAL E FUNCIONAL)
#include "orbit_camera.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/constants.hpp>
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
              m_projectionMatrix(1.0f), // Inicializa a matriz de projeção como identidade
              m_lastMouseX(0.0), m_lastMouseY(0.0), m_firstMouse(true)
        {
            Engine::Log::Info("OrbitCamera: Construtor chamado.");
        }

        void OrbitCamera::setTarget(const glm::vec3 &target)
        {
            target_ = target;

            // CORREÇÃO FINAL PARA O FRAME 0:
            // setRotation usa pitch_ e yaw_ e os aplica ao Target.
            // REMOVA: resetMouseState();
            // setRotation(pitch_, yaw_); // <--- Chamamos setRotation para forçar o recálculo.

            // A chamada a setRotation já deve ser o suficiente.

            Engine::Log::Debug(std::format("OrbitCamera: Target definido para {}", glm::to_string(target_)));
        }

        // REMOVIDO: O método InitializePosition deve ser removido

        void OrbitCamera::setDistance(float distance)
        {
            distance_ = glm::clamp(distance, m_minDistance, m_maxDistance);
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
                return;
            }

            float deltaX = static_cast<float>(xpos - m_lastMouseX);
            float deltaY = static_cast<float>(m_lastMouseY - ypos);

            m_lastMouseX = xpos;
            m_lastMouseY = ypos;

            const float sensitivity = 0.003f;

            yaw_ -= deltaX * sensitivity;
            pitch_ += deltaY * sensitivity;

            pitch_ = glm::clamp(pitch_, m_minPitch, m_maxPitch);

            Engine::Log::Trace(std::format("OrbitCamera: Mouse moved (deltaX: {}, deltaY: {}). Pitch: {}, Yaw: {}.", deltaX, deltaY, pitch_, yaw_));
        }

        void OrbitCamera::processScroll(double yOffset)
        {
            setDistance(distance_ - static_cast<float>(yOffset));
            Engine::Log::Debug(std::format("OrbitCamera: Processed scroll. New distance: {}.", distance_));
        }

        void OrbitCamera::processKeyboard(CameraMovement direction, float deltaTime)
        {
            // Intencionalmente vazio
        }

        glm::mat4 OrbitCamera::getViewMatrix() const
        {
            float x = distance_ * cosf(pitch_) * sinf(yaw_);
            float y = distance_ * sinf(pitch_);
            float z = distance_ * cosf(pitch_) * cosf(yaw_);

            glm::vec3 cameraPos = target_ - glm::vec3(x, y, z);

            if (cameraPos.y < 0.05f)
            {
                cameraPos.y = 0.05f;
            }

            return glm::lookAt(cameraPos, target_, glm::vec3(0, 1, 0));
        }

        float OrbitCamera::getZoom() const
        {
            return m_zoom;
        }

        const glm::vec3 &OrbitCamera::getPosition() const
        {
            static glm::vec3 calculatedPos;
            float x = distance_ * cosf(pitch_) * sinf(yaw_);
            float y = distance_ * sinf(pitch_);
            float z = distance_ * cosf(pitch_) * cosf(yaw_);
            calculatedPos = target_ - glm::vec3(x, y, z);
            return calculatedPos;
        }

        // NOVO: getTarget() implementado
        glm::vec3 OrbitCamera::getTarget() const
        {
            return target_;
        }

        float OrbitCamera::getYaw() const
        {
            return glm::degrees(yaw_);
        }

        glm::vec3 OrbitCamera::getForwardVector() const
        {
            return glm::normalize(glm::vec3(sin(yaw_), 0.0f, cos(yaw_)));
        }

        glm::vec3 OrbitCamera::getRightVector() const
        {
            return glm::normalize(glm::cross(getForwardVector(), glm::vec3(0.0f, 1.0f, 0.0f)));
        }

        void OrbitCamera::setPosition(const glm::vec3 &position)
        {
            target_ = position;
        }

        void OrbitCamera::resetMouseState()
        {
            m_firstMouse = true;
            Engine::Log::Debug("OrbitCamera: Mouse state reset (m_firstMouse = true).");
        }

        void OrbitCamera::setZoom(float zoom_value)
        {
            m_zoom = glm::clamp(zoom_value, 1.0f, 90.0f);
            Engine::Log::Debug(std::format("OrbitCamera: Zoom (FOV) definido para {}.", m_zoom));
        }

        void OrbitCamera::setYaw(float yaw_degrees)
        {
            yaw_ = glm::radians(yaw_degrees);
            Engine::Log::Trace(std::format("OrbitCamera: Yaw definido para {} graus.", yaw_degrees));
        }

        void OrbitCamera::setProjectionMatrix(float fov, float aspectRatio, float nearPlane, float farPlane)
        {
            m_projectionMatrix = glm::perspective(glm::radians(fov), aspectRatio, nearPlane, farPlane);
        }

        const glm::mat4 &OrbitCamera::getProjectionMatrix() const
        {
            return m_projectionMatrix;
        }

        void OrbitCamera::setDistanceLimits(float min, float max)
        {
            m_minDistance = min;
            m_maxDistance = max;
            setDistance(distance_);
        }

        void OrbitCamera::setPitchLimits(float minDegrees, float maxDegrees)
        {
            m_minPitch = glm::radians(minDegrees);
            m_maxPitch = glm::radians(maxDegrees);
            pitch_ = glm::clamp(pitch_, m_minPitch, m_maxPitch);
        }

        glm::mat4 OrbitCamera::getViewProjectionMatrix() const
        {
            return getProjectionMatrix() * getViewMatrix();
        }

    } // namespace Camera
} // namespace Engine