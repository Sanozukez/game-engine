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
            // **** MUDANÇA CRUCIAL AQUI: Declarar getYaw ****
            float getYaw() const override;

            // Métodos específicos da OrbitCamera (SEM 'override', pois não estão em ICamera)
            void setDistance(float distance);
            void setRotation(float pitch, float yaw);

        private:
            glm::vec3 target_;
            float distance_;
            float pitch_;
            float yaw_;
            float m_zoom;

            double m_lastMouseX = 0.0;
            double m_lastMouseY = 0.0;
            bool m_firstMouse = true;
        };

    } // namespace Camera
} // namespace Engine