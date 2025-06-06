// src/app/app.h
#pragma once

#include "scene.h"
#include <memory> 
#include "./../../engine/window/window.h" 

namespace Engine { 
    class Renderer; 
    namespace Camera { 
        class ICamera; 
        class FreeCamera; // Necessário para instanciar FreeCamera
        class OrbitCamera; // Necessário para instanciar OrbitCamera
    } 
    class Scene; 
} 

class App {
public:
    App();
    ~App(); 
    void run();

    // REMOVIDO: void switchCamera();

private:
    std::unique_ptr<Engine::Window> m_window;
    std::unique_ptr<Engine::Renderer> m_renderer; 
    Engine::Scene scene; 

    // REMOVIDO: bool m_isFreeCameraActive = true; 
};