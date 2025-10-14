// // engine/input/i_mouse_button_listener.h

#pragma once

#include <GLFW/glfw3.h>

namespace Engine {
namespace Input {

/**
 * @brief Interface de Listener para eventos de clique do mouse (buttons).
 * * Implementa ISP, separando botões do movimento e scroll.
 */
class IMouseButtonListener {
public:
    virtual ~IMouseButtonListener() = default;

    /**
     * @brief Lida com o pressionar e soltar dos botões do mouse.
     * * @param button O código do botão (e.g., GLFW_MOUSE_BUTTON_LEFT).
     * @param action O estado (e.g., GLFW_PRESS/RELEASE).
     */
    virtual void handleMouseButtonInput(int button, int action) = 0;
};

} // namespace Input
} // namespace Engine