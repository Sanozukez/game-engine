// engine/render/renderer.cpp
#include "renderer.h"
#include "./../window/window.h" // Inclua a classe Window para acesso aos métodos
#include "./shader.h"        // Inclua a classe Shader para configurar uniforms
#include "./../core/log.h"   // Inclua o sistema de log
#include "./camera/icamera.h" // Use a interface ICamera
#include "./../../src/app/scene.h" // Inclua Scene para renderizar

#include <glad/gl.h> // Para comandos OpenGL
#include <glm/gtc/matrix_transform.hpp> // Para glm::perspective
#include <glm/gtx/string_cast.hpp> // Para glm::to_string, se usado em logs aqui

namespace Engine {

Renderer::Renderer(const Window& window, const Camera::ICamera& camera)
    : m_window(window), m_camera(camera) {
    Engine::Log::Info("Renderer: Construtor chamado.");
    // A matriz de projeção será configurada em setProjectionMatrix
}

Renderer::~Renderer() {
    Engine::Log::Info("Renderer: Destrutor chamado.");
}

void Renderer::render(const Scene& scene) {
    clearScreen();
    // **** NOVO: Chamar configureViewport e setProjectionMatrix a cada frame ****
    // Isso garante que o viewport e a projeção se ajustem a qualquer redimensionamento.
    configureViewport();
    setProjectionMatrix(m_camera.getZoom(), 0.1f, 100.0f); // Use o FOV da câmera atual

    // Obtém as matrizes de visão e projeção
    glm::mat4 view = m_camera.getViewMatrix();
    glm::mat4 projection = m_projectionMatrix;

    scene.render(projection, view); 
}

void Renderer::setClearColor(float r, float g, float b, float a) {
    glClearColor(r, g, b, a);
    Engine::Log::Debug(std::format("Renderer: Cor de limpeza definida para ({},{},{},{}).", r,g,b,a));
}

void Renderer::setProjectionMatrix(float fov, float nearPlane, float farPlane) {
    // Usa as dimensões ATUAIS da janela para o aspect ratio
    float aspectRatio = m_window.getAspectRatio(); 
    m_projectionMatrix = glm::perspective(glm::radians(fov), aspectRatio, nearPlane, farPlane);
    Engine::Log::Debug(std::format("Renderer: Matriz de projeção configurada. FOV: {}, Aspect: {}, Near: {}, Far: {}.", fov, aspectRatio, nearPlane, farPlane));
}

void Renderer::configureViewport() {
    // Usa as dimensões ATUAIS da janela para o viewport
    glViewport(0, 0, m_window.getWidth(), m_window.getHeight());
    Engine::Log::Debug(std::format("Renderer: Viewport configurado para {}x{}.", m_window.getWidth(), m_window.getHeight()));
}

void Renderer::clearScreen() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

} // namespace Engine