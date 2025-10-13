// // src/app/app.cpp

#include "app.h"
#include "./../../engine/window/window.h"
#include "./../../engine/core/log.h"
#include "./../../engine/ecs/world.h"

// Includes de dependências que App::run() precisa para criar e orquestrar
#include "./../../engine/camera/free_camera.h"
#include "./../../engine/camera/orbit_camera.h"
#include "./../../engine/render/renderer.h"
#include "./../../engine/core/config_manager.h"
#include "./../../engine/input/input_manager.h"

// NOVO: Inclui a classe AppSetup, que contém toda a lógica de inicialização
#include "app_setup.h" 

#include <glm/gtx/string_cast.hpp>
#include <thread>
#include <chrono>
#include <glm/glm.hpp>
#include <iostream>
#include <format>
#include <limits>
#include <GLFW/glfw3.h> // Necessário para as funções GLFW no APP


using namespace Engine::ECS;

// ----------------------------------------------------------------------------------
// Construtor e Destrutor
// ----------------------------------------------------------------------------------

App::App() : m_window(nullptr), m_renderer(nullptr), m_gameWorld(std::make_unique<World>())
{
    Engine::Core::Log::Info("[App] Construtor chamado");
}

App::~App() = default;


// ----------------------------------------------------------------------------------
// FUNÇÃO PRINCIPAL: ORQUESTRAÇÃO DO FLUXO (CLEAN)
// ----------------------------------------------------------------------------------

void App::run()
{
    Engine::Core::Log::Info("[App] Iniciando aplicação");

    // --- CARREGAMENTO DE CONFIGURAÇÃO BASE ---
    auto &config = Engine::Core::ConfigManager::
Get();
    if (!config.load("config/engine_settings.json"))
    {
        Engine::Core::Log::Critical("[App] Falha ao carregar configurações essenciais. Encerrando.");
        return;
    }

    // --- 1. INICIALIZAÇÃO DA JANELA ---
    // ESTA LÓGICA DEVE FICAR AQUI, POIS CRIA MEMBROS DE App (m_window)
    Engine::WindowConfig winConfig;
    winConfig.width = config.getValue<int>("window.width", 1280);
    // ... (resto da configuração da janela) ...
    winConfig.height = config.getValue<int>("window.height", 720);
    winConfig.title = config.getValue<std::string>("window.title", "Default Title");
    winConfig.resizable = config.getValue<bool>("window.resizable", false);
    winConfig.fullscreen = config.getValue<bool>("window.fullscreen", false);
    winConfig.maximized = false;
    
    try { m_window = std::make_unique<Engine::Window>(winConfig); }
    catch (const std::exception &e) { Engine::Core::Log::Critical(std::format("[App] Erro fatal na inicialização da janela: {}", e.what())); return; }

    GLFWwindow *glfwWindow = m_window->getGLFWWindow();
    if (!glfwWindow) { return; }
    Engine::Input::InputManager::Get().ProcessInput(glfwWindow);


    // --- 2. INICIALIZAÇÃO DA CÂMERA E RENDERER ---
    // ESTA LÓGICA DEVE FICAR AQUI, POIS CRIA MEMBROS DE App (m_mainCamera, m_renderer)
    std::string cameraType = config.getValue<std::string>("camera.default_type", "orbit");

    if (cameraType == "free")
    {
        m_mainCamera = std::make_unique<Engine::Camera::FreeCamera>();
    }
    else // Assumimos "orbit"
    {
        // O ponteiro é movido diretamente para m_mainCamera
        m_mainCamera = std::make_unique<Engine::Camera::OrbitCamera>();
    }
    Engine::Core::Log::Info(std::format("[App] Câmera inicializada a partir do JSON: {}", cameraType));
    
    m_renderer = std::make_unique<Engine::Render::Renderer>(*m_window, *m_mainCamera.get());

    // --- 3. ORQUESTRAÇÃO DO SETUP (DELEGADO PARA AppSetup) ---
    Engine::Render::Renderer &rendererRef = *m_renderer;
    Engine::Camera::ICamera &cameraRef = *m_mainCamera.get();
    
    // ** CHAMADA 1: CONFIGURAÇÃO DE CÂMERA/LUZ **
    if (!Engine::Core::AppSetup::InitializeConfiguration(config, rendererRef, cameraRef, glfwWindow)) {
        Engine::Core::Log::Critical("[App] Falha na configura├º├úo de Luz/C├ómera. Encerrando.");
        return;
    }
    
    // ** CHAMADA 2: CRIAÇÃO DE ENTIDADES, LOAD E ADIÇÃO DE SISTEMAS **
    if (!Engine::Core::AppSetup::InitializeECS(*m_gameWorld.get(), cameraRef, rendererRef, glfwWindow)) {
        Engine::Core::Log::Critical("[App] Falha ao inicializar o ECS/World. Encerrando.");
        return;
    }
    
    Engine::Core::Log::Info("[App] Setup completo. Iniciando Game Loop.");


    // === LOOP DE RENDERIZAÇÃO ===
    float lastFrame = 0.0f;
    float deltaTime = 0.0f;

    while (!m_window->shouldClose())
    {
        float currentFrame = m_window->getTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        auto &inputManager = Engine::Input::InputManager::Get();

        // Lógica de Teclas de Controle (F11, F10) - Permanece no App
        if (inputManager.IsKeyPressed(GLFW_KEY_F11))
        {
            m_window->isMaximized() ? m_window->restore() : m_window->maximize();
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        }

        if (inputManager.IsKeyPressed(GLFW_KEY_F10))
        {
            int defaultWidth = config.getValue<int>("window.width", 1280);
            int defaultHeight = config.getValue<int>("window.height", 720);

            m_window->setResolutionAndMode(defaultWidth, defaultHeight, !m_window->isFullscreen());
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        }

        // MUNDO ECS: Chama a lógica de todos os sistemas
        m_gameWorld->update(deltaTime);

        m_window->swapBuffersAndPollEvents();
    }

    Engine::Core::Log::Info("[App] Encerrando aplicação.");
    glfwTerminate();
}

// FIM: Não há mais código de implementação complexo aqui!