// engine/render/camera/orbit_camera.h
#pragma once

#include "icamera.h"
#include <glm/glm.hpp>

namespace Engine
{
    namespace Camera
    {
        class OrbitCamera : public ICamera
        {
        public:
            OrbitCamera();

            // Métodos sobrescritos existentes
            void setTarget(const glm::vec3 &target) override;
            void setPosition(const glm::vec3 &position) override;
            void processMouseMovement(double xpos, double ypos) override;
            void processScroll(double yOffset) override;
            void processKeyboard(CameraMovement direction, float deltaTime) override;
            glm::mat4 getViewMatrix() const override;
            float getZoom() const override;
            const glm::vec3 &getPosition() const override;
            glm::vec3 getForwardVector() const override;
            glm::vec3 getRightVector() const override;
            void resetMouseState() override;
            void setZoom(float zoom_value) override;
            void setYaw(float yaw_degrees) override;
            float getYaw() const override;
            void setProjectionMatrix(float fov_degrees, float aspectRatio, float nearPlane, float farPlane) override;
            const glm::mat4 &getProjectionMatrix() const override;

            // Métodos específicos da OrbitCamera
            void setDistance(float distance);
            void setRotation(float pitch, float yaw);

            // --- NOVOS MÉTODOS PÚBLICOS DE CONFIGURAÇÃO ---
            void setDistanceLimits(float min, float max);
            void setPitchLimits(float minDegrees, float maxDegrees);

        private:
            glm::vec3 target_;
            float distance_;
            float pitch_;
            float yaw_;
            float m_zoom;

            double m_lastMouseX;
            double m_lastMouseY;
            bool m_firstMouse;
            
            glm::mat4 m_projectionMatrix;

            // --- NOVOS MEMBROS PRIVADOS PARA OS LIMITES ---
            float m_minDistance = 2.0f;
            float m_maxDistance = 25.0f;
            float m_minPitch = glm::radians(-20.0f); // Limites em radianos para uso interno
            float m_maxPitch = glm::radians(85.0f);
        };

    } // namespace Camera
} // namespace Engine