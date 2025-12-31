// src/editor/editor_app.cpp
// Implementação do Editor Visual - Versão Básica Inicial

#include "editor_app.h"
#include "render/opengl_types.h"
#include "window/window.h"
#include "core/log.h"

#include <GLFW/glfw3.h>

EditorApp::EditorApp() {
    Engine::Core::Log::Info("[Editor] Construtor chamado");
    initWindow();
}

EditorApp::~EditorApp() {
    Engine::Core::Log::Info("[Editor] Destrutor chamado");
}

void EditorApp::initWindow() {
    // Configuração da janela
    Engine::WindowConfig config;
    config.width = 1600;
    config.height = 900;
    config.title = "Game Engine Editor v0.1 - Tela Inicial";
    config.resizable = true;
    config.fullscreen = false;
    config.maximized = true;  // Inicia maximizada
    
    m_window = std::make_unique<Engine::Window>(config);
    Engine::Core::Log::Info("[Editor] Janela criada");
}

void EditorApp::initImGui() {
    // TODO: Implementar quando ImGuiLayer estiver pronto
    Engine::Core::Log::Info("[Editor] ImGui - aguardando implementação");
}

void EditorApp::setupStyle() {
    // TODO: Implementar estilo quando ImGui estiver pronto
    Engine::Core::Log::Info("[Editor] Estilo - aguardando implementação");
}

void EditorApp::run() {
    Engine::Core::Log::Info("[Editor] Iniciando loop principal");
    
    while (!m_window->shouldClose()) {
        // Clear com cor de fundo agradável
        glClearColor(0.15f, 0.16f, 0.18f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        // TODO: Renderizar UI aqui quando ImGui estiver implementado
        // Por enquanto, apenas mantém a janela aberta
        
        // Swap buffers e poll events
        m_window->swapBuffersAndPollEvents();
    }
    
    Engine::Core::Log::Info("[Editor] Loop principal encerrado");
}

void EditorApp::renderMenuBar() {
    // TODO: Implementar menu bar com ImGui
}

void EditorApp::renderStatusBar() {
    // TODO: Implementar status bar com ImGui
}

void EditorApp::renderMainContent() {
    // TODO: Implementar conteúdo principal com ImGui
}
