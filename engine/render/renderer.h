// engine/render/renderer.h
#pragma once

#include <glm/glm.hpp>
#include <memory>
#include <string>
#include "./camera/icamera.h"

namespace Engine {
    class Window;
    class Scene;
}

namespace Engine {

class Renderer {
public:
    // --- MUDANÇA: O construtor e o membro da câmera agora usam uma referência NÃO-constante ---
    Renderer(const Window& window, Camera::ICamera& camera);
    ~Renderer();

    void render(const Scene& scene);
    void setClearColor(float r, float g, float b, float a);
    
    // Este método agora atua como um comando para configurar a projeção na câmera
    void updateProjectionMatrix();

private:
    const Window& m_window;
    // --- MUDANÇA: Referência não-constante para poder chamar setProjectionMatrix ---
    Camera::ICamera& m_camera;

    // --- REMOVIDO: A câmera agora gerencia sua própria matriz de projeção ---
    // glm::mat4 m_projectionMatrix;

    void configureViewport();
    void clearScreen();
};

} // namespace Engine