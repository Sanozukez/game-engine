// src/app/app.cpp
#include "app.h"
#include "./../../engine/window/window.h" 
#include "./../../engine/render/renderer.h" 
#include "input.h"                       
#include "scene.h"                       
#include "./../../engine/core/log.h"     

// **** MUDANÇA AQUI: Incluir explicitamente o InputManager.h (agora no namespace correto) ****
#include "./../../engine/input/input_manager.h" 
#include <thread> 
#include <chrono> 

#include <glm/glm.hpp> 
#include <iostream>    

App::App() : m_window(nullptr), m_renderer(nullptr), scene() { 
    Engine::Log::Info("[App] Construtor chamado");
}

App::~App() = default;

void App::run() {
    Engine::Log::Info("[App] Iniciando aplica├º├úo");

    Engine::WindowConfig winConfig;
    winConfig.width = 1280; 
    winConfig.height = 720;
    winConfig.title = "Game Engine MMORPG";
    winConfig.resizable = false; 
    winConfig.maximized = false; 
    winConfig.fullscreen = false; 

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
    m_renderer->setProjectionMatrix(45.0f, 0.1f, 500.0f); 
    Engine::Log::Info("[App] Renderer inicializado e configurado.");

    setup_application_input(glfwWindow, scene.getCamera(), scene, 0.0f); 
    Engine::Log::Info("[App] Callbacks de input configurados via InputManager.");

    scene.initialize(); 
    Engine::Log::Info("[App] Cena inicializada com sucesso.");

    float lastFrame = 0.0f;
    float deltaTime = 0.0f;

    while (!m_window->shouldClose()) { 
        float currentFrame = m_window->getTime(); 
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        if (Engine::Input::InputManager::Get().IsKeyPressed(GLFW_KEY_F11)) {
            if (m_window->isMaximized()) {
                m_window->restore();
            } else {
                m_window->maximize();
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(200)); 
        }
        
        if (Engine::Input::InputManager::Get().IsKeyPressed(GLFW_KEY_F10)) {
            m_window->setResolutionAndMode(Engine::DEFAULT_WINDOW_WIDTH, Engine::DEFAULT_WINDOW_HEIGHT, !m_window->isFullscreen());
            std::this_thread::sleep_for(std::chrono::milliseconds(200)); 
        }

        // **** MUDANÇA AQUI: Chamar Scene::update com InputManager (agora no namespace correto) ****
        scene.update(deltaTime, static_cast<const Engine::Input::InputManager&>(Engine::Input::InputManager::Get())); 
        m_renderer->render(scene); 

        m_window->swapBuffersAndPollEvents(); 
    }

    Engine::Log::Info("[App] Encerrando aplica├º├úo.");
    glfwTerminate(); 
}