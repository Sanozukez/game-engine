// // engine/input/i_scroll_listener.h

#pragma once

namespace Engine {
namespace Input {

/**
 * @brief Interface de Listener para eventos de rolagem (scroll wheel).
 * * Implementa o ISP: Separa a rolagem do movimento do cursor.
 */
class IScrollListener {
public:
    virtual ~IScrollListener() = default;

    /**
     * @brief Lida com a rolagem da roda do mouse.
     * * @param yOffset O delta de rolagem no eixo Y.
     */
    virtual void handleScrollInput(double yOffset) = 0;
};

} // namespace Input
} // namespace Engine