// engine/window/window.cpp
#include "window.h"
#include <glad/gl.h>
#include "./../core/log.h"
#include <format> 
#include "./../core/config.h" 

namespace Engine {

Window::Window(const WindowConfig& config)
    : m_window(nullptr), 
      m_width(config.width), 
      m_height(config.height), 
      m_title(config.title),
      m_resizable(config.resizable),
      m_maximizedOnStart(config.maximized),
      m_isFullscreen(config.fullscreen), 
      m_windowedX(0), m_windowedY(0), 
      m_windowedWidth(config.width), m_windowedHeight(config.height) { 

    if (!initGLFW()) {
        Engine::Log::Critical(std::format("Window: Falha ao inicializar GLFW para a janela '{}'.", m_title));
        return;
    }

    setResolutionAndMode(config.width, config.height, config.fullscreen);

    if (!m_window) { 
        Engine::Log::Critical(std::format("Window: Falha crítica ao criar a janela inicial '{}'.", m_title));
        glfwTerminate();
        return;
    }

    glfwMakeContextCurrent(m_window);
    Engine::Log::Info(std::format("Window: Contexto OpenGL criado e ativado para '{}'.", m_title));

    if (!loadGLAD()) {
        Engine::Log::Critical("Window: Falha ao carregar GLAD.");
        glfwDestroyWindow(m_window);
        glfwTerminate();
        return;
    }
    Engine::Log::Info("Window: GLAD carregado com sucesso.");

    setupOpenGLDefaults();

    glfwSetWindowUserPointer(m_window, this); 
    glfwSetFramebufferSizeCallback(m_window, framebuffer_size_callback);
    Engine::Log::Info("Window: Callback de redimensionamento do framebuffer registrado.");

    if (!m_isFullscreen && m_maximizedOnStart) {
        glfwMaximizeWindow(m_window);
        Engine::Log::Info(std::format("Window: Janela '{}' iniciada maximizada.", m_title));
    }
}

Window::~Window() {
    if (m_window) {
        Engine::Log::Info(std::format("Window: Destruindo janela '{}'.", m_title));
        glfwDestroyWindow(m_window);
    }
}

bool Window::initGLFW() {
    if (!glfwInit()) {
        Engine::Log::Error("Window: Erro ao inicializar GLFW.");
        return false;
    }
    Engine::Log::Info("Window: GLFW inicializado.");

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    
    glfwWindowHint(GLFW_RESIZABLE, m_resizable ? GLFW_TRUE : GLFW_FALSE);

    Engine::Log::Info("Window: Hints do GLFW configurados para OpenGL 4.5 Core Profile.");
    Engine::Log::Info(std::format("Window: Redimensionável: {}", m_resizable ? "Sim" : "Não"));
    return true;
}

bool Window::loadGLAD() {
    if (!gladLoadGL(glfwGetProcAddress)) {
        Engine::Log::Error("Window: Erro ao carregar GLAD.");
        return false;
    }
    return true;
}

void Window::setupOpenGLDefaults() {
    glEnable(GL_DEPTH_TEST);
    Engine::Log::Info("Window: GL_DEPTH_TEST ativado.");

    glViewport(0, 0, m_width, m_height);
    Engine::Log::Info(std::format("Window: Viewport inicial configurado para {}x{}.", m_width, m_height));

    glfwSwapInterval(1); // V-Sync
    Engine::Log::Info("Window: V-Sync ativado (glfwSwapInterval(1)).");
}

bool Window::shouldClose() const {
    return glfwWindowShouldClose(m_window);
}

void Window::swapBuffersAndPollEvents() {
    glfwSwapBuffers(m_window);
    glfwPollEvents();
}

double Window::getTime() const {
    return glfwGetTime();
}

void Window::framebuffer_size_callback(GLFWwindow* window, int width, int int_height) { 
    Window* self = static_cast<Window*>(glfwGetWindowUserPointer(window));
    if (self) {
        self->m_width = width;
        self->m_height = int_height; 
        Engine::Log::Info(std::format("Window: Framebuffer redimensionado para {}x{}. Atualizando dimensões internas.", width, int_height));
    }
}

void Window::maximize() {
    if (m_window && !m_isFullscreen) { 
        glfwMaximizeWindow(m_window);
        Engine::Log::Info(std::format("Window: Janela '{}' maximizada.", m_title));
    }
}

void Window::restore() {
    if (m_window) {
        glfwRestoreWindow(m_window);
        Engine::Log::Info(std::format("Window: Janela '{}' restaurada.", m_title));
    }
}

bool Window::isMaximized() const {
    return m_window && (glfwGetWindowAttrib(m_window, GLFW_MAXIMIZED) == GLFW_TRUE);
}

bool Window::isFullscreen() const {
    return m_isFullscreen;
}

void Window::setResolutionAndMode(int width, int height, bool fullscreen) {
    if (!m_window) {
        m_width = width;
        m_height = height;
        m_isFullscreen = fullscreen;
        
        if (m_isFullscreen) {
            GLFWmonitor* primaryMonitor = glfwGetPrimaryMonitor();
            if (!primaryMonitor) {
                Engine::Log::Error("Window: Não foi possível obter o monitor primário para criar janela fullscreen.");
                return; 
            }
            const GLFWvidmode* mode = glfwGetVideoMode(primaryMonitor);
            m_window = glfwCreateWindow(width, height, m_title.c_str(), primaryMonitor, nullptr);
            if (m_window) { 
                m_width = width; 
                m_height = height;
            } else {
                Engine::Log::Error(std::format("Window: Falha ao criar janela fullscreen {}x{}. Tentando modo janela.", width, height));
                m_isFullscreen = false;
                m_window = glfwCreateWindow(width, height, m_title.c_str(), nullptr, nullptr);
            }
        } else {
            m_window = glfwCreateWindow(width, height, m_title.c_str(), nullptr, nullptr);
        }
        
        if (m_window) {
            glfwGetWindowPos(m_window, &m_windowedX, &m_windowedY); 
            glfwGetWindowSize(m_window, &m_windowedWidth, &m_windowedHeight); 
            Engine::Log::Info(std::format("Window: Janela inicial criada em {} ({}) {}x{}.", 
                                         m_isFullscreen ? "Fullscreen" : "Windowed", m_title, m_width, m_height));
        } else {
            Engine::Log::Critical("Window: Falha irreversível ao criar janela.");
        }
        return; 
    }


    if (fullscreen == m_isFullscreen && width == m_width && height == m_height) {
        Engine::Log::Info("Window: Resolução e modo já são os desejados. Nenhuma alteração.");
        return; 
    }

    if (fullscreen && !m_isFullscreen) {
        Engine::Log::Info(std::format("Window: Alterando para Fullscreen {}x{}.", width, height));
        glfwGetWindowPos(m_window, &m_windowedX, &m_windowedY); 
        glfwGetWindowSize(m_window, &m_windowedWidth, &m_windowedHeight);

        enterFullscreen(glfwGetPrimaryMonitor(), width, height, 0); 
        m_isFullscreen = true;
    } else if (!fullscreen && m_isFullscreen) {
        Engine::Log::Info(std::format("Window: Alterando para Windowed {}x{}.", width, height));
        exitFullscreen(m_windowedX, m_windowedY, width, height);
        m_isFullscreen = false;
    } else if (!fullscreen && !m_isFullscreen) {
        Engine::Log::Info(std::format("Window: Redimensionando para Windowed {}x{}.", width, height));
        glfwSetWindowSize(m_window, width, height);
        glfwSetWindowPos(m_window, m_windowedX, m_windowedY); 
    } else if (fullscreen && m_isFullscreen) {
        Engine::Log::Info(std::format("Window: Alterando resolução Fullscreen para {}x{}.", width, height));
        enterFullscreen(glfwGetPrimaryMonitor(), width, height, 0);
    }

    m_width = width;
    m_height = height;

    Engine::Log::Info(std::format("Window: Resolução e modo alterados para {} ({}x{}).",
                                  m_isFullscreen ? "Fullscreen" : "Windowed", m_width, m_height));
}

void Window::enterFullscreen(GLFWmonitor* monitor, int width, int height, int refreshRate) {
    if (glfwGetWindowAttrib(m_window, GLFW_ICONIFIED)) {
        glfwRestoreWindow(m_window);
    }
    glfwSetWindowMonitor(m_window, monitor, 0, 0, width, height, refreshRate);
}

void Window::exitFullscreen(int x, int y, int width, int height) {
    glfwSetWindowMonitor(m_window, nullptr, x, y, width, height, 0);
}

} // namespace Engine