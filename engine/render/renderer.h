// engine/render/renderer.h
#pragma once

#include <glm/glm.hpp>
#include <memory> 
#include <string>

// Inclua a interface ICamera aqui, pois ela será usada como tipo de referência.
#include "./camera/icamera.h" 

// Forward declarations para as classes que o Renderer vai interagir
namespace Engine {
    class Window;      
    class Scene;       
    // REMOVIDO: namespace Camera { class FreeCamera; } -> Não é mais necessário, ICamera já é incluída
}

class Shader; 

namespace Engine {

class Renderer {
public:
    // **** MUDANÇA AQUI: O construtor agora recebe uma const Engine::Camera::ICamera& ****
    Renderer(const Window& window, const Camera::ICamera& camera); 
    ~Renderer();

    // Método principal de renderização do frame
    void render(const Scene& scene);

    // Métodos para configuração de renderização (SRP do Renderer)
    void setClearColor(float r, float g, float b, float a);
    void setProjectionMatrix(float fov, float nearPlane, float farPlane);
    // Outros métodos de configuração global de renderização aqui

private:
    const Window& m_window; 
    // **** MUDANÇA AQUI: O membro da câmera agora é uma referência const para a interface ICamera ****
    const Camera::ICamera& m_camera; 

    // Matriz de projeção, gerenciada pelo Renderer
    glm::mat4 m_projectionMatrix;

    // Métodos auxiliares
    void configureViewport();
    void clearScreen();
};

} // namespace Engine