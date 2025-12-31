// // engine/input/i_keyboard_listener.h

#pragma once

// O GLFW é necessário para as constantes de tecla (GLFW_KEY_*, GLFW_PRESS, etc.)
#include <GLFW/glfw3.h>

namespace Engine {
namespace Input {

/**
 * @brief Interface de Listener para eventos de teclado.
 * * Implementa o ISP: Garante que classes que SÓ precisam de input de teclado
 * não sejam obrigadas a implementar métodos de mouse ou scroll.
 */
class IKeyboardListener {
public:
    virtual ~IKeyboardListener() = default;

    /**
     * @brief Lida com eventos de tecla (pressionar/soltar).
     * * @param key O código da tecla (e.g., GLFW_KEY_W).
     * @param action O estado da tecla (e.g., GLFW_PRESS, GLFW_RELEASE).
     */
    virtual void handleKeyInput(int key, int action) = 0;
};

} // namespace Input
} // namespace Engine