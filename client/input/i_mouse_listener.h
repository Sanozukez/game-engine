// // engine/input/i_mouse_listener.h

#pragma once

namespace Engine {
namespace Input {

/**
 * @brief Interface de Listener para eventos de movimento do mouse (cursor).
 * * Implementa o ISP: Separa o tracking do cursor do scroll.
 */
class IMouseListener {
public:
    virtual ~IMouseListener() = default;

    /**
     * @brief Lida com o movimento do cursor.
     * * @param xpos Posição X atual do cursor na janela.
     * @param ypos Posição Y atual do cursor na janela.
     */
    virtual void handleMouseMovement(double xpos, double ypos) = 0;
};

} // namespace Input
} // namespace Engine