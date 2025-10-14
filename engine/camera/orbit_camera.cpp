// engine/camera/orbit_camera.cpp
#include "orbit_camera.h"
#include "../core/config_manager.h"
#include "../core/log.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtx/string_cast.hpp>
#include <algorithm>
#include <format>
#include "camera_math.h" // util de ângulos e checagens
#include <glm/gtx/norm.hpp>

namespace Engine
{
    namespace Camera
    {

        OrbitCamera::OrbitCamera()
        {
            Core::Log::Info("OrbitCamera: ctor");
        }

        void OrbitCamera::applyExternalConfig(const Core::ConfigManager &config)
        {
            const float minDist = config.getValue<float>("camera.orbit.min_distance", 3.0f);
            const float maxDist = config.getValue<float>("camera.orbit.max_distance", 13.0f);
            const float minPitch = config.getValue<float>("camera.orbit.min_pitch_degrees", -55.0f);
            const float maxPitch = config.getValue<float>("camera.orbit.max_pitch_degrees", 15.0f);

            setDistanceLimits(minDist, maxDist);
            setPitchLimits(minPitch, maxPitch);

            Core::Log::Info("[OrbitCamera] Limits applied from config");
        }

        glm::mat4 OrbitCamera::getViewMatrix() const
        {
            rebuildCachedPosition();

            // direção para o alvo
            glm::vec3 dir = glm::normalize(m_target - m_cachedPos);

            // up base
            glm::vec3 up(0.0f, 1.0f, 0.0f);

            // evita colinearidade: se cross quase zero, ajusta levemente o up
            glm::vec3 right = glm::cross(dir, up);
            if (glm::length2(right) < 1e-10f)
            {
                up = glm::normalize(glm::cross(glm::vec3(1.0f, 0.0f, 0.0f), dir));
                if (glm::length2(up) < 1e-10f)
                {
                    up = glm::vec3(0.0f, 1.0f, 0.0f);
                }
            }
            else
            {
                up = glm::normalize(glm::cross(right, dir));
            }

            return glm::lookAt(m_cachedPos, m_target, up);
        }

        glm::mat4 OrbitCamera::getProjectionMatrix() const
        {
            return m_projection; // por valor, consistente com a interface
        }

        glm::mat4 OrbitCamera::getViewProjectionMatrix() const
        {
            return getProjectionMatrix() * getViewMatrix();
        }

        float OrbitCamera::getZoom() const
        {
            return m_fovDeg;
        }

        const glm::vec3 &OrbitCamera::getPosition() const
        {
            rebuildCachedPosition();
            return m_cachedPos;
        }

        glm::vec3 OrbitCamera::getForwardVector() const
        {
            // direções no plano XZ (niveladas), úteis para strafe do target
            return glm::normalize(glm::vec3(std::sin(m_yawRad), 0.0f, std::cos(m_yawRad)));
        }

        glm::vec3 OrbitCamera::getRightVector() const
        {
            return glm::normalize(glm::cross(getForwardVector(), glm::vec3(0, 1, 0)));
        }

        float OrbitCamera::getYaw() const
        {
            return glm::degrees(m_yawRad);
        }

        float OrbitCamera::getPitch() const
        {
            return glm::degrees(m_pitchRad);
        }

        glm::vec3 OrbitCamera::getTarget() const
        {
            return m_target;
        }

        void OrbitCamera::setPosition(const glm::vec3 &position)
        {
            // Na orbit, “position” significa orbitar o alvo → interpretamos como novo alvo.
            m_target = position;
        }

        void OrbitCamera::setTarget(const glm::vec3 &target)
        {
            m_target = target;
            // Core::Log::Debug(std::format("OrbitCamera: target={}", glm::to_string(target)));
        }

        void OrbitCamera::setZoom(float zoom_value)
        {
            m_fovDeg = std::clamp(zoom_value, 1.0f, 90.0f);
            Core::Log::Debug(std::format("OrbitCamera: FOV={} deg", m_fovDeg));
        }

        void OrbitCamera::setYaw(float yaw_degrees)
        {
            m_yawRad = glm::radians(yaw_degrees);
            // wrap para manter estável
            if (m_yawRad > glm::pi<float>())
                m_yawRad -= glm::two_pi<float>();
            if (m_yawRad < -glm::pi<float>())
                m_yawRad += glm::two_pi<float>();
        }

        void OrbitCamera::setPitch(float pitch_degrees)
        {
            m_pitchRad = glm::radians(pitch_degrees);
            clampPitch();
        }

        void OrbitCamera::setProjectionMatrix(float fov_degrees,
                                              float aspectRatio,
                                              float nearPlane,
                                              float farPlane)
        {
            m_fovDeg = fov_degrees;
            m_projection = glm::perspective(glm::radians(fov_degrees), aspectRatio, nearPlane, farPlane);
        }

        void OrbitCamera::processKeyboard(CameraMovement direction, float dt)
        {
            const float speed = 3.5f * dt;

            glm::vec3 fwd = getForwardVector();
            glm::vec3 right = getRightVector();

            // mantém nivelado
            fwd.y = 0.0f;
            right.y = 0.0f;
            fwd = glm::normalize(fwd);
            right = glm::normalize(right);

            switch (direction)
            {
            case LEFT:
                m_target -= right * speed;
                break;
            case RIGHT:
                m_target += right * speed;
                break;
            case FORWARD:
                m_target += fwd * speed;
                break;
            case BACKWARD:
                m_target -= fwd * speed;
                break;
            default:
                break; // UP/DOWN/rotates não aplicam no alvo aqui
            }
        }

        void OrbitCamera::processMouseMovement(double xpos, double ypos)
        {
            if (m_firstMouse)
            {
                m_lastMouseX = xpos;
                m_lastMouseY = ypos;
                m_firstMouse = false;
                return; // ignora primeiro delta
            }

            float deltaX = static_cast<float>(xpos - m_lastMouseX);
            float deltaY = static_cast<float>(m_lastMouseY - ypos);
            m_lastMouseX = xpos;
            m_lastMouseY = ypos;

            // --- anti-spike: satura deltas por frame ---
            const float maxDelta = 100.0f; // pixels/frame
            deltaX = glm::clamp(deltaX, -maxDelta, +maxDelta);
            deltaY = glm::clamp(deltaY, -maxDelta, +maxDelta);

            const float sensitivity = 0.003f;

            // sentido “natural”: mover mouse p/ direita → yaw positivo (ou troque o sinal se preferir)
            m_yawRad -= deltaX * sensitivity;
            m_pitchRad += deltaY * sensitivity;

            // clamp de pitch nos limites já em radianos
            clampPitch();

            // wrap de yaw para [-pi, pi] (evita crescimento indefinido)
            if (m_yawRad > glm::pi<float>())
                m_yawRad -= glm::two_pi<float>();
            if (m_yawRad < -glm::pi<float>())
                m_yawRad += glm::two_pi<float>();
        }

        void OrbitCamera::processScroll(double yOffset)
        {
            setDistance(m_distance - static_cast<float>(yOffset));
            Core::Log::Debug(std::format("OrbitCamera: distance={}", m_distance));
        }

        void OrbitCamera::setDistanceLimits(float minD, float maxD)
        {
            m_minDistance = minD;
            m_maxDistance = maxD;
            setDistance(m_distance); // re-clamp atual
        }

        void OrbitCamera::setPitchLimits(float minPitchDeg, float maxPitchDeg)
        {
            m_minPitchRad = glm::radians(minPitchDeg);
            m_maxPitchRad = glm::radians(maxPitchDeg);
            clampPitch();
        }

        void OrbitCamera::setDistance(float distance)
        {
            m_distance = std::clamp(distance, m_minDistance, m_maxDistance);
            if (m_distance < 0.05f)
                m_distance = 0.05f; // piso duro para evitar lookAt degenerado
        }

        void OrbitCamera::setRotation(float pitchRad, float yawRad)
        {
            m_pitchRad = pitchRad;
            m_yawRad = yawRad;
            clampPitch();
        }

        // --- helpers ---
        void OrbitCamera::clampPitch()
        {
            m_pitchRad = std::clamp(m_pitchRad, m_minPitchRad, m_maxPitchRad);
        }

        void OrbitCamera::rebuildCachedPosition() const
        {
            const float x = m_distance * std::cos(m_pitchRad) * std::sin(m_yawRad);
            const float y = m_distance * std::sin(m_pitchRad);
            const float z = m_distance * std::cos(m_pitchRad) * std::cos(m_yawRad);

            m_cachedPos = m_target - glm::vec3(x, y, z);

            // evita lookAt degenerar
            if (m_cachedPos.y < 0.05f)
                m_cachedPos.y = 0.05f;
        }

    } // namespace Camera
} // namespace Engine
