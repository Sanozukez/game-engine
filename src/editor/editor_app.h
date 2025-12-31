// src/editor/editor_app.h
// Aplicação do Editor Visual

#pragma once

#include <memory>

// Forward declarations
namespace Engine {
    class Window;
}

class EditorApp {
public:
    EditorApp();
    ~EditorApp();

    void run();

private:
    void initWindow();
    void initImGui();
    void setupStyle();
    
    void renderMenuBar();
    void renderStatusBar();
    void renderMainContent();
    
    // Estado
    std::unique_ptr<Engine::Window> m_window;
    
    bool m_showDemoWindow = false;
    float m_statusBarHeight = 25.0f;
};
