// engine/render/renderer.cpp
#include "./shader.h"
#include "renderer.h"
#include "safety_guards.h"
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

                // --- Guards anti-NaN/Inf com cache local (evita teleporte) ---
                static glm::mat4 s_lastGoodProj = glm::mat4(1.0f);
                static glm::mat4 s_lastGoodView = glm::mat4(1.0f);

                // Projection: tenta corrigir e, se necessário, usa a última válida
                if (!Engine::Render::Safety::finiteMat4(projection))
                {
                    m_camera.setProjectionMatrix(
                        glm::clamp(m_camera.getZoom(), 20.0f, 100.0f),
                        std::max(0.0001f, m_window.getAspectRatio()),
                        0.1f, 500.0f);
                    projection = m_camera.getProjectionMatrix();
                    if (!Engine::Render::Safety::finiteMat4(projection))
                    {
                        projection = Engine::Render::Safety::finiteMat4(s_lastGoodProj) ? s_lastGoodProj : glm::mat4(1.0f);
                    }
                }

                // View: reconstrói com lookAt seguro e, se necessário, usa a última válida
                if (!Engine::Render::Safety::finiteMat4(view))
                {
                    const glm::vec3 eye = m_camera.getPosition();
                    const glm::vec3 center = eye + m_camera.getForwardVector();
                    const glm::vec3 up = glm::vec3(0, 1, 0);
                    view = Engine::Render::Safety::safeLookAt(eye, center, up);
                    if (!Engine::Render::Safety::finiteMat4(view))
                    {
                        view = Engine::Render::Safety::finiteMat4(s_lastGoodView) ? s_lastGoodView : glm::mat4(1.0f);
                    }
                }

                // Atualiza o cache se este frame está válido
                if (Engine::Render::Safety::finiteMat4(projection))
                    s_lastGoodProj = projection;
                if (Engine::Render::Safety::finiteMat4(view))
                    s_lastGoodView = view;

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

            // Engine::Core::Log::Trace(std::format("Renderer: Matriz de projeção da câmera atualizada. FOV: {}, Aspect: {}.", fov, aspectRatio));
        }

        void Renderer::configureViewport()
        {
            glViewport(0, 0, m_window.getWidth(), m_window.getHeight());
            // Engine::Core::Log::Trace(std::format("Renderer: Viewport configurado para {}x{}.", m_window.getWidth(), m_window.getHeight()));
        }

        void Renderer::clearScreen()
        {
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        }
    } // namespace Render
} // namespace Engine
