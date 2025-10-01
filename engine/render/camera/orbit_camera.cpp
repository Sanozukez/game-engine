// engine/render/camera/orbit_camera.cpp
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

            // Limita o ângulo vertical (pitch) para evitar que a câmera vire de cabeça para baixo
            pitch_ = glm::clamp(pitch_, -1.5f, 1.5f); // ~85 graus para cima e para baixo

            Engine::Log::Trace(std::format("OrbitCamera: Mouse moved (deltaX: {}, deltaY: {}). Pitch: {}, Yaw: {}.", deltaX, deltaY, pitch_, yaw_));
        }

        void OrbitCamera::processScroll(double yOffset)
        {
            setDistance(distance_ - static_cast<float>(yOffset));
            Engine::Log::Debug(std::format("OrbitCamera: Processed scroll. New distance: {}.", distance_));
        }

        void OrbitCamera::processKeyboard(CameraMovement direction, float deltaTime)
        {
            // Intencionalmente vazio, pois o movimento do personagem é controlado pela classe PlayerCharacter.
        }

        glm::mat4 OrbitCamera::getViewMatrix() const
        {
            // Usa matemática esférica para calcular a posição da câmera com base nos ângulos e na distância do alvo.
            // As funções sinf e cosf esperam ângulos em radianos, o que está correto pois pitch_ e yaw_ são armazenados em radianos.
            float x = distance_ * cosf(pitch_) * sinf(yaw_);
            float y = distance_ * sinf(pitch_);
            float z = distance_ * cosf(pitch_) * cosf(yaw_);

            glm::vec3 cameraPos = target_ - glm::vec3(x, y, z);
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

        float OrbitCamera::getYaw() const
        {
            // --- CORREÇÃO APLICADA ---
            // Converte o yaw (armazenado internamente em radianos) para graus antes de retornar.
            // Isso é crucial para que a classe PlayerCharacter possa interpretar o ângulo corretamente.
            return glm::degrees(yaw_);
        }

        glm::vec3 OrbitCamera::getForwardVector() const
        {
            // --- CORREÇÃO APLICADA ---
            // Usa yaw_ diretamente, pois ele já está em radianos. A chamada incorreta a glm::radians() foi removida.
            // O vetor é projetado no plano horizontal (Y=0) para o movimento do personagem no chão.
            return glm::normalize(glm::vec3(sin(yaw_), 0.0f, cos(yaw_)));
        }

        glm::vec3 OrbitCamera::getRightVector() const
        {
            // Esta função já estava correta e agora funciona como esperado, pois depende da getForwardVector() corrigida.
            return glm::normalize(glm::cross(getForwardVector(), glm::vec3(0.0f, 1.0f, 0.0f)));
        }

        void OrbitCamera::setPosition(const glm::vec3 &position)
        {
            Engine::Log::Warn("OrbitCamera: setPosition called. Na câmera de órbita, é preferível usar setTarget().");
            // Em uma câmera de órbita, mudar a "posição" geralmente significa mudar o alvo.
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
            // Converte os graus recebidos para radianos para armazenamento interno.
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

    } // namespace Camera
} // namespace Engine