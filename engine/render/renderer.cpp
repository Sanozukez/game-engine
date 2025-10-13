// engine/render/renderer.cpp
#include "./shader.h"
#include "renderer.h"
#include "../math/transform_utils.h"
#include "./../window/window.h"
#include "./../core/log.h"
#include "../camera/icamera.h"
#include "../ecs/components/transform_component.h" // Para usar Transform no submit
#include "../asset/asset_manager.h"
#include "../asset/model.h"
#include "../core/path_utils.h"

#include "opengl_types.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/string_cast.hpp>

namespace Engine
{
    namespace Render
    {

        // --- MUDANÇA: O construtor aceita uma referência não-constante ---
        Renderer::Renderer(const Window &window, Camera::ICamera &camera)
            : m_window(window), m_camera(camera)
        {

            std::string vs_path = Engine::resolveEnginePath("engine/shaders/basic.vert").string();
            std::string fs_path = Engine::resolveEnginePath("engine/shaders/basic.frag").string();
            m_defaultShader = std::make_unique<Shader>(vs_path.c_str(), fs_path.c_str());

            Engine::Core::Log::Info(std::format("Renderer: Construtor chamado. Shader PBR 'basic' inicializado."));

            // FUTURE: Você precisará carregar os arquivos de shader reais aqui.
        }

        // --- IMPLEMENTAÇÃO DOS NOVOS SETTERS DE LUZ (SOLUÇÃO C2039) ---

        void Renderer::setGlobalLightPos(const glm::vec3 &pos)
        {
            m_globalLightPos = pos;
            Engine::Core::Log::Debug(std::format("Renderer: Light Position set to {}.", glm::to_string(pos)));
        }

        void Renderer::setGlobalLightColor(const glm::vec3 &color)
        {
            m_globalLightColor = color;
            Engine::Core::Log::Debug(std::format("Renderer: Light Color set to {}.", glm::to_string(color)));
        }

        void Renderer::setGlobalLightIntensity(float intensity)
        {
            m_globalLightIntensity = intensity;
            Engine::Core::Log::Debug(std::format("Renderer: Light Intensity set to {}.", intensity));
        }

        Engine::Render::Shader &Renderer::getActiveShader()
        {
            // Retorna o shader padrão que foi inicializado no construtor.
            return *m_defaultShader;
        }

        Renderer::~Renderer()
        {
            Engine::Core::Log::Info("Renderer: Destrutor chamado.");
        }

        void Renderer::setClearColor(float r, float g, float b, float a)
        {
            glClearColor(r, g, b, a);
            Engine::Core::Log::Debug(std::format("Renderer: Cor de limpeza definida para ({},{},{},{}).", r, g, b, a));
        }

        // **********************************************
        // NOVOS MÉTODOS DE CONTROLE DE FRAME (ECS)
        // **********************************************

        void Renderer::beginScene()
        {
            // Movendo a lógica de início de frame para cá
            clearScreen();
            configureViewport();
            updateProjectionMatrix();

            // FUTURE: Aqui você deve usar a Câmera para obter a ViewMatrix
            // e passá-la para o Shader Global. Ex: m_camera.getViewMatrix();
        }

        void Renderer::endScene()
        {
            // Sem lógica complexa aqui por enquanto.
        }

        // **********************************************
        // IMPLEMENTAÇÃO ECS: submit (Recebe DADOS puros)
        // **********************************************

        void Renderer::submit(uint32_t assetID, const ECS::Component::Transform &transform)
        {
            std::shared_ptr<Engine::Asset::Model> model = Engine::Asset::AssetManager::Get().getModel(assetID);

            if (model)
            {
                // 1. Obter Shader
                Engine::Render::Shader &shader = getActiveShader();
                shader.use();

                // 2. Obter Matrizes
                glm::mat4 projection = m_camera.getProjectionMatrix();
                glm::mat4 view = m_camera.getViewMatrix();

                // --- CORREÇÃO FINAL: Usar a matriz de transformação completa (T, R e S) ---
                // A modelMatrix é declarada e inicializada UMA ÚNICA VEZ aqui.
                glm::mat4 modelMatrix = Engine::Math::getTransformMatrix(transform);

                // 3. ENVIAR MATRIZES
                shader.setMat4("uProjection", projection);
                shader.setMat4("uView", view);
                shader.setMat4("uModel", modelMatrix); // <-- AGORA COM ROTAÇÃO E ESCALA

                // NOVO: Setar as propriedades globais da Fonte de Luz (MIGRADO DO HARDCODE)
                // Assumimos que o Shader tem as uniforms uLightColor e uLightIntensity.
                shader.setVec3("uLightPos", m_globalLightPos);
                shader.setVec3("uLightColor", m_globalLightColor);
                shader.setFloat("uLightIntensity", m_globalLightIntensity);

                // 4. Setar posição da câmera (para iluminação)
                shader.setVec3("uViewPos", m_camera.getPosition());

                // 5. Chamar a função de desenho
                model->draw(shader);

                shader.unuse();
            }
        }

        void Renderer::updateProjectionMatrix()
        {
            float aspectRatio = m_window.getAspectRatio();
            float fov = m_camera.getZoom(); // Usa o FOV (zoom) atual da câmera

            // Comando para a câmera recalcular e armazenar sua matriz de projeção
            m_camera.setProjectionMatrix(fov, aspectRatio, 0.1f, 500.0f);

            Engine::Core::Log::Trace(std::format("Renderer: Matriz de projeção da câmera atualizada. FOV: {}, Aspect: {}.", fov, aspectRatio));
        }

        void Renderer::configureViewport()
        {
            glViewport(0, 0, m_window.getWidth(), m_window.getHeight());
            Engine::Core::Log::Trace(std::format("Renderer: Viewport configurado para {}x{}.", m_window.getWidth(), m_window.getHeight()));
        }

        void Renderer::clearScreen()
        {
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        }
    } // namespace Render
} // namespace Engine
