// // src/app/app.h (CORREÇÃO FINAL DE NAMESPACE)

#pragma once

#include <memory> 
#include "./../../engine/window/window.h" 

// 1. FORWARD DECLARATIONS PARA A ENGINE
namespace Engine { 

    class Window;

    // DECLARAÇÃO CORRETA DO RENDERER NO NAMESPACE Render
    namespace Render {
        class Renderer; 
        class ICamera; // ICamera pode estar em Render::Camera
    }
    
    namespace Camera { 
        class ICamera; // Se a ICamera estiver neste namespace, mantenha
    } 
    
    // 2. FORWARD DECLARATION CRUCIAL PARA O ECS
    namespace ECS {
        class World; 
    }
} 

// Forward declaration necessário para a função de setup
struct GLFWwindow; // Declarar a struct GLFWwindow

class App {
public:
    App();
    ~App(); 
    void run();

private:
    std::unique_ptr<Engine::Window> m_window;
    std::unique_ptr<Engine::Render::Renderer> m_renderer; 
    std::unique_ptr<Engine::ECS::World> m_gameWorld;
    std::unique_ptr<Engine::Camera::ICamera> m_mainCamera; 
};