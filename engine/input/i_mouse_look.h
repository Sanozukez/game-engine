// // engine/input/i_mouse_look.h
#pragma once
#include <GLFW/glfw3.h>

namespace Engine
{
    namespace Input
    {
        // MouseLook: captura deltas de mouse para rotação "travada no centro",
        // ignorando o primeiro frame após RMB down para evitar o salto.
        class MouseLook
        {
        public:
            void init(GLFWwindow *w, float sensitivity = 0.12f)
            {
                window = w;
                this->sensitivity = sensitivity;
            }

            void onRMBDown()
            {
                rotating = true;
                int winW = 0, winH = 0;
                glfwGetWindowSize(window, &winW, &winH);
                centerX = winW * 0.5;
                centerY = winH * 0.5;

                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
#ifdef GLFW_RAW_MOUSE_MOTION
                if (glfwRawMouseMotionSupported())
                    glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
#endif

                // Posiciona no centro e marca para ignorar o primeiro delta
                glfwSetCursorPos(window, centerX, centerY);
                firstFrame = true;
            }

            void onRMBUp()
            {
                rotating = false;
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
#ifdef GLFW_RAW_MOUSE_MOTION
                if (glfwRawMouseMotionSupported())
                    glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_FALSE);
#endif
            }

            // Chame 1x por frame. Retorna true se houver delta a aplicar.
            bool update(float &outDx, float &outDy)
            {
                if (!rotating)
                    return false;

                if (firstFrame)
                {
                    // Debounce: ainda não mover no primeiro frame
                    glfwSetCursorPos(window, centerX, centerY);
                    firstFrame = false;
                    return false;
                }

                double x = 0.0, y = 0.0;
                glfwGetCursorPos(window, &x, &y);

                const double dx = x - centerX;
                const double dy = y - centerY;

                if (dx == 0.0 && dy == 0.0)
                {
                    return false; // nada para aplicar
                }

                outDx = static_cast<float>(dx) * sensitivity;
                outDy = static_cast<float>(-dy) * sensitivity; // pitch invertido (ajuste se quiser)

                // Re-centra para que o próximo frame meça novo delta
                glfwSetCursorPos(window, centerX, centerY);
                return true;
            }

            bool isRotating() const { return rotating; }

        private:
            GLFWwindow *window = nullptr;
            bool rotating = false;
            bool firstFrame = false;
            double centerX = 0.0, centerY = 0.0;
            float sensitivity = 0.12f;
        };

    } // namespace Input
} // namespace Engine