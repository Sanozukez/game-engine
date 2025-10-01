#include "app.h"
#include "./../../engine/window/window.h"
#include "./../../engine/render/renderer.h"
#include "input.h"
#include "scene.h"
#include "./../../engine/core/log.h"
#include "./../../engine/core/config_manager.h"
#include "./../../engine/input/input_manager.h"
#include "./../../engine/physics/raycaster.h"
#include "./../../engine/game/player_character.h"
#include <glm/gtx/string_cast.hpp>

#include <thread>
#include <chrono>
#include <glm/glm.hpp>
#include <iostream>
#include <format>


App::App() : m_window(nullptr), m_renderer(nullptr), scene() {
    Engine::Log::Info("[App] Construtor chamado");
}

App::~App() = default;

void App::run() {
    Engine::Log::Info("[App] Iniciando aplicação");

    if (!Engine::ConfigManager::Get().load("config/engine_settings.json")) {
        Engine::Log::Critical("[App] Falha ao carregar configurações essenciais. Encerrando.");
        return;
    }

    auto& config = Engine::ConfigManager::Get();

    Engine::WindowConfig winConfig;
    winConfig.width = config.getValue<int>("window.width", 1280);
    winConfig.height = config.getValue<int>("window.height", 720);
     winConfig.title = config.getValue<std::string>("window.title", "Default Title");
    winConfig.resizable = config.getValue<bool>("window.resizable", false);
    winConfig.fullscreen = config.getValue<bool>("window.fullscreen", false);
    winConfig.maximized = false; // Geralmente não é uma config inicial

    try {
        m_window = std::make_unique<Engine::Window>(winConfig);
    } catch (const std::exception& e) {
        Engine::Log::Critical(std::format("[App] Erro fatal na inicialização da janela: {}", e.what()));
        return;
    }

    GLFWwindow* glfwWindow = m_window->getGLFWWindow();
    if (!glfwWindow) {
        Engine::Log::Critical("[App] Ponteiro GLFWwindow inválido após criação da janela.");
        return;
    }

    m_renderer = std::make_unique<Engine::Renderer>(*m_window, scene.getCamera());
    m_renderer->setClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    Engine::Log::Info("[App] Renderer inicializado e configurado.");

    setup_application_input(glfwWindow, scene.getCamera(), scene, 0.0f);
    Engine::Log::Info("[App] Callbacks de input configurados via InputManager.");

    scene.initialize();
    Engine::Log::Info("[App] Cena inicializada com sucesso.");

    Engine::Physics::Raycaster raycaster;

    Engine::Input::InputManager::Get().RegisterCallback(
        Engine::Input::InputEvent::MouseButtonPressed,
        GLFW_MOUSE_BUTTON_LEFT,
        [this, &raycaster, glfwWindow](const Engine::Input::InputEventData& data) {
            Engine::Game::PlayerCharacter* player = scene.getPlayer();
            if (player) {
                double mouseX, mouseY;
                glfwGetCursorPos(glfwWindow, &mouseX, &mouseY);
                int screenWidth, screenHeight;
                glfwGetWindowSize(glfwWindow, &screenWidth, &screenHeight);
                raycaster.update(mouseX, mouseY, screenWidth, screenHeight, scene.getCamera());
                glm::vec3 intersectionPoint;
                if (raycaster.getIntersectionWithPlane(0.0f, intersectionPoint)) {
                    Engine::Log::Info(std::format("Clique de mouse (via callback) detectado! Movendo para: {}", glm::to_string(intersectionPoint)));
                    player->moveTo(intersectionPoint);
                }
            }
        }
    );

    float lastFrame = 0.0f;
    float deltaTime = 0.0f;

    while (!m_window->shouldClose()) {
        float currentFrame = m_window->getTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        auto& inputManager = Engine::Input::InputManager::Get();

        if (inputManager.IsKeyPressed(GLFW_KEY_F11)) {
            m_window->isMaximized() ? m_window->restore() : m_window->maximize();
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        }

        // --- MUDANÇA AQUI ---
        if (inputManager.IsKeyPressed(GLFW_KEY_F10)) {
            // Pega a instância do ConfigManager
            auto& config = Engine::ConfigManager::Get();

            // Busca a resolução padrão a partir do arquivo de configuração
            int defaultWidth = config.getValue<int>("window.width", 1280);
            int defaultHeight = config.getValue<int>("window.height", 720);

            // Usa os valores do config para restaurar a resolução
            m_window->setResolutionAndMode(defaultWidth, defaultHeight, !m_window->isFullscreen());
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        }
        
        scene.update(deltaTime, inputManager);
        m_renderer->render(scene);
        m_window->swapBuffersAndPollEvents();
    }

    Engine::Log::Info("[App] Encerrando aplicação.");
    glfwTerminate();
}