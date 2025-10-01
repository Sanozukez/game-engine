// engine/render/renderer.cpp
#include "renderer.h"
#include "./../window/window.h"
#include "./shader.h"
#include "./../core/log.h"
#include "./camera/icamera.h"
#include "./../../src/app/scene.h"

#include <glad/gl.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/string_cast.hpp>

namespace Engine
{

    // --- MUDANÇA: O construtor aceita uma referência não-constante ---
    Renderer::Renderer(const Window &window, Camera::ICamera &camera)
        : m_window(window), m_camera(camera)
    {
        Engine::Log::Info("Renderer: Construtor chamado.");
    }

    Renderer::~Renderer()
    {
        Engine::Log::Info("Renderer: Destrutor chamado.");
    }

    void Renderer::render(const Scene &scene)
    {
        clearScreen();
        configureViewport();

        // --- MUDANÇA: Atualiza a matriz de projeção da câmera a cada frame ---
        updateProjectionMatrix();

        // Obtém as matrizes de visão e projeção diretamente da câmera
        glm::mat4 view = m_camera.getViewMatrix();
        glm::mat4 projection = m_camera.getProjectionMatrix();

        // --- LOG DE DEBUG ADICIONADO ---
        // Vamos usar Log::Trace para não poluir o console depois.
        Engine::Log::Trace(std::format("Renderer::render() - View Matrix:\n{}", glm::to_string(view)));
        Engine::Log::Trace(std::format("Renderer::render() - Projection Matrix:\n{}", glm::to_string(projection)));
        // --- FIM DO LOG DE DEBUG ---

        scene.render(projection, view);
    }

    void Renderer::setClearColor(float r, float g, float b, float a)
    {
        glClearColor(r, g, b, a);
        Engine::Log::Debug(std::format("Renderer: Cor de limpeza definida para ({},{},{},{}).", r, g, b, a));
    }

    // --- MUDANÇA: Este método agora configura a câmera ---
    void Renderer::updateProjectionMatrix()
    {
        float aspectRatio = m_window.getAspectRatio();
        float fov = m_camera.getZoom(); // Usa o FOV (zoom) atual da câmera

        // Comando para a câmera recalcular e armazenar sua matriz de projeção
        m_camera.setProjectionMatrix(fov, aspectRatio, 0.1f, 500.0f);

        Engine::Log::Trace(std::format("Renderer: Matriz de projeção da câmera atualizada. FOV: {}, Aspect: {}.", fov, aspectRatio));
    }

    void Renderer::configureViewport()
    {
        glViewport(0, 0, m_window.getWidth(), m_window.getHeight());
        Engine::Log::Trace(std::format("Renderer: Viewport configurado para {}x{}.", m_window.getWidth(), m_window.getHeight()));
    }

    void Renderer::clearScreen()
    {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

} // namespace Engine