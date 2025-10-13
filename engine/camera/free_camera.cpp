// engine/camera/free_camera.cpp
#include "free_camera.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/string_cast.hpp>
#include "./../../../engine/core/log.h"
#include "../core/config_manager.h"
#include <format>

namespace Engine
{
    namespace Camera
    {

        FreeCamera::FreeCamera()
            : position(glm::vec3(0.0f, 1.0f, 5.0f)),
              worldUp(glm::vec3(0.0f, 1.0f, 0.0f)),
              yaw(0.0f),
              pitch(0.0f),
              speed(5.0f),
              sensitivity(0.1f),
              zoom(45.0f),
              m_projectionMatrix(1.0f), // --- NOVO: Inicializa a matriz de projeção ---
              m_lastMouseX(0.0),
              m_lastMouseY(0.0),
              m_firstMouse(true)
        {
            updateVectors();
        }

        void FreeCamera::setPosition(const glm::vec3 &position)
        {
            this->position = position;
            Engine::Core::Log::Debug(std::format("FreeCamera: Posição definida para {}", glm::to_string(position)));
        }

        const glm::vec3 &FreeCamera::getPosition() const
        {
            return position;
        }

        glm::vec3 FreeCamera::getTarget() const
        {
            // Retorna a posição atual + o vetor forward (para onde a câmera está olhando)
            // Usamos a variável de membro correta que você tem: position.
            return position + front;
        }

        float FreeCamera::getZoom() const
        {
            return zoom;
        }

        float FreeCamera::getYaw() const
        {
            return yaw;
        }

        void FreeCamera::updateVectors()
        {
            glm::vec3 newFront;
            newFront.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
            newFront.y = sin(glm::radians(pitch));
            newFront.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
            front = glm::normalize(newFront);

            right = glm::normalize(glm::cross(front, worldUp));
            up = glm::normalize(glm::cross(right, front));
        }

        void FreeCamera::processMouseMovement(double xpos, double ypos)
        {
            if (m_firstMouse)
            {
                m_lastMouseX = xpos;
                m_lastMouseY = ypos;
                m_firstMouse = false;
                Engine::Core::Log::Debug(std::format("FreeCamera: First mouse movement handled. Initializing lastX: {}, lastY: {}", m_lastMouseX, m_lastMouseY));
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

        void FreeCamera::processKeyboard(CameraMovement direction, float deltaTime)
        {
            float velocity = speed * deltaTime;

            switch (direction)
            {
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
                Engine::Core::Log::Trace("FreeCamera: Ignoring ROTATE_LEFT keyboard input.");
                break;
            case ROTATE_RIGHT:
                Engine::Core::Log::Trace("FreeCamera: Ignoring ROTATE_RIGHT keyboard input.");
                break;
            }
        }

        glm::mat4 FreeCamera::getViewMatrix() const
        {
            return glm::lookAt(position, position + front, up);
        }

        void FreeCamera::resetMouseState()
        {
            m_firstMouse = true;
            Engine::Core::Log::Debug("FreeCamera: Mouse state reset (m_firstMouse = true).");
        }

        void FreeCamera::processScroll(double yOffset)
        {
            // FreeCamera não usa scroll para zoom, então deixamos vazio.
        }

        void FreeCamera::setTarget(const glm::vec3 &target)
        {
            Engine::Core::Log::Warn("FreeCamera: setTarget called, but control is via position. Ignoring or adjusting.");
        }

        void FreeCamera::setZoom(float zoom_value)
        {
            zoom = glm::clamp(zoom_value, 1.0f, 90.0f);
            Engine::Core::Log::Debug(std::format("FreeCamera: Zoom (FOV) definido para {}.", zoom));
        }

        void FreeCamera::setYaw(float yaw_degrees)
        {
            yaw = yaw_degrees;
            updateVectors();
            Engine::Core::Log::Trace(std::format("FreeCamera: Yaw definido para {}.", yaw_degrees));
        }

        // **** NOVOS: Implementação de getForwardVector e getRightVector ****
        glm::vec3 FreeCamera::getForwardVector() const
        {
            return glm::normalize(glm::vec3(front.x, 0.0f, front.z));
        }

        glm::vec3 FreeCamera::getRightVector() const
        {
            return glm::normalize(glm::cross(getForwardVector(), glm::vec3(0.0f, 1.0f, 0.0f)));
        }

        void FreeCamera::setProjectionMatrix(float fov_degrees, float aspectRatio, float nearPlane, float farPlane)
        {
            m_projectionMatrix = glm::perspective(glm::radians(fov_degrees), aspectRatio, nearPlane, farPlane);
        }

        const glm::mat4 &FreeCamera::getProjectionMatrix() const
        {
            return m_projectionMatrix;
        }

        glm::mat4 FreeCamera::getViewProjectionMatrix() const
        {
            // Combina a matriz de projeção (Perspectiva) com a matriz de visão (LookAt)
            return m_projectionMatrix * getViewMatrix();
        }

        // IMPLEMENTAÇÃO DE CONTRATO (NO-OPs para FreeCamera)
        void FreeCamera::setDistanceLimits(float minDistance, float maxDistance) {
            // FreeCamera não tem limites de distância.
            Engine::Core::Log::Warn("[FreeCamera] Tentativa de aplicar setDistanceLimits; não aplicável.");
        }

        void FreeCamera::setPitchLimits(float minPitchDegrees, float maxPitchDegrees) {
            // FreeCamera não tem limites de pitch.
            Engine::Core::Log::Warn("[FreeCamera] Tentativa de aplicar setPitchLimits; não aplicável.");
        }

        void FreeCamera::applyExternalConfig(const Engine::Core::ConfigManager &config) {
            // Este método satisfaz o contrato. Pode ser estendido para ler velocidade, etc.
            // Para o escopo do erro, basta garantir que ele existe.
            Engine::Core::Log::Info("[FreeCamera] Configuração externa aplicada (limites ignorados).");
            
            // Exemplo de como você poderia ler a velocidade futura (se necessário):
            // float speed = config.getValue<float>("camera.free.speed", 10.0f);
            // m_movementSpeed = speed; 
        }

    } // namespace Camera
} // namespace Engine