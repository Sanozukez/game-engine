// engine/window/window.h
#pragma once

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <string>

namespace Engine {

// Estrutura para configurar a janela (já existente)
struct WindowConfig {
    int width;
    int height;
    std::string title;
    bool resizable;    
    bool maximized;    
    bool fullscreen;   
};

class Window {
public:
    // Construtor: Agora aceita uma struct de configuração (já existente)
    Window(const WindowConfig& config);
    // Destrutor (já existente)
    ~Window();

    // Getters (já existentes)
    GLFWwindow* getGLFWWindow() const { return m_window; }
    bool shouldClose() const;
    void swapBuffersAndPollEvents();
    int getWidth() const { return m_width; }
    int getHeight() const { return m_height; }
    float getAspectRatio() const { return static_cast<float>(m_width) / static_cast<float>(m_height); }
    double getTime() const;
    void maximize();
    void restore();
    bool isMaximized() const;
    bool isFullscreen() const; // Já existente

    // **** NOVO: Métodos para alterar a janela em tempo de execução ****
    // Muda a resolução e o modo (janela/tela cheia).
    // Usado pelos menus de configuração.
    void setResolutionAndMode(int width, int height, bool fullscreen);

private:
    GLFWwindow* m_window;
    int m_width;
    int m_height;
    std::string m_title;
    bool m_resizable;
    bool m_maximizedOnStart;
    bool m_isFullscreen; 

    int m_windowedX;
    int m_windowedY;
    int m_windowedWidth;
    int m_windowedHeight;

    static void framebuffer_size_callback(GLFWwindow* window, int width, int height);

    bool initGLFW();
    bool createWindowAndContext(const WindowConfig& config); 
    bool loadGLAD();
    void setupOpenGLDefaults();

    // **** NOVO: Funções auxiliares para os modos de janela ****
    void enterFullscreen(GLFWmonitor* monitor, int width, int height, int refreshRate);
    void exitFullscreen(int x, int y, int width, int height);
};

} // namespace Engine