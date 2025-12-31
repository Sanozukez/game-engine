// // engine/core/app_setup.h

#pragma once

#include "../../../engine/ecs/world.h"
#include "../../../client/render/renderer.h"
#include "../../../client/camera/icamera.h"
#include "../../../engine/core/config_manager.h"
#include <GLFW/glfw3.h> 

namespace Engine {
namespace Core {

class AppSetup {
public:
    // Construtor Default
    AppSetup() = default;

    // 1. Inicializa Janela, Configurações de Câmera e Luz (Leitura de JSON)
    static bool InitializeConfiguration(
        Engine::Core::ConfigManager& config, 
        Engine::Render::Renderer& rendererRef, 
        Engine::Camera::ICamera& cameraRef,
        GLFWwindow* glfwWindow);

    // 2. Cria Entidades, Adiciona Componentes, Adiciona Sistemas e Carrega o Mundo
    static bool InitializeECS(
        Engine::ECS::World& world, 
        Engine::Camera::ICamera& cameraRef, 
        Engine::Render::Renderer& rendererRef, 
        GLFWwindow* glfwWindow);
};

} // namespace Core
} // namespace Engine