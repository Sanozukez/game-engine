// engine/render/camera/icamera.h
#pragma once

#include <glm/glm.hpp>

namespace Engine
{
    namespace Camera
    {

        enum CameraMovement
        {
            FORWARD,
            BACKWARD,
            LEFT,
            RIGHT,
            UP,
            DOWN,
            ROTATE_LEFT,
            ROTATE_RIGHT
        };

        class ICamera
        {
        public:
            virtual ~ICamera() = default;

            virtual glm::mat4 getViewMatrix() const = 0;
            virtual float getZoom() const = 0;
            virtual const glm::vec3 &getPosition() const = 0;
            virtual glm::vec3 getForwardVector() const = 0;
            virtual glm::vec3 getRightVector() const = 0;

            virtual void processKeyboard(CameraMovement direction, float deltaTime) = 0;
            virtual void processMouseMovement(double xpos, double ypos) = 0;
            virtual void processScroll(double yOffset) = 0;

            virtual void setPosition(const glm::vec3 &position) = 0;
            virtual void setTarget(const glm::vec3 &target) = 0;
            virtual void resetMouseState() = 0;
            virtual void setZoom(float zoom_value) = 0;
            virtual void setYaw(float yaw_degrees) = 0;
            // **** MUDANÇA CRUCIAL AQUI: Adicionar getYaw() à interface ICamera ****
            virtual float getYaw() const = 0;
        };

    } // namespace Camera
} // namespace Engine