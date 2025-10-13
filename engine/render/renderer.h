// engine/render/renderer.h
#pragma once

#include <glm/glm.hpp>
#include <memory>
#include <string>
#include "../camera/icamera.h"
#include "../ecs/components/transform_component.h"
#include <cstdint> // Para uint32_t

namespace Engine
{
    class Window;

    namespace ECS
    {
        namespace Component
        {
            struct Transform; // Forward declaration de struct se TransformComponent for struct pura
        }
    }

    namespace Render
    {
        class Shader;
    }
}

namespace Engine
{
    namespace Render
    {
        class Renderer
        {
        public:
            // Chamadas de controle de Frame
            void beginScene();

            void endScene();

            //  Recebe os DADOS puros do ECS e desenha o objeto.
            void submit(uint32_t assetID, const ECS::Component::Transform &transform);

            //  Retorna o Shader Ativo
            Engine::Render::Shader &getActiveShader();

            // O construtor e o membro da câmera agora usam uma referência NÃO-constante ---
            Renderer(const Window &window, Camera::ICamera &camera);
            ~Renderer();

            void setClearColor(float r, float g, float b, float a);

            // Este método agora atua como um comando para configurar a projeção na câmera
            void updateProjectionMatrix();

            // NOVOS SETTERS DE LUZ (CORREÇÃO DE ERROS C2039)
            void setGlobalLightPos(const glm::vec3& pos);
            void setGlobalLightColor(const glm::vec3& color);
            void setGlobalLightIntensity(float intensity);

        private:
            const Window &m_window;
            Camera::ICamera &m_camera;

            // MEMBRO NECESSÁRIO: O Renderer é o responsável pelo ownership do shader padrão.
            std::unique_ptr<Engine::Render::Shader> m_defaultShader; // <--- AGORA DECLARADO!

            glm::vec3 m_globalLightPos = glm::vec3(50.0f, 50.0f, 50.0f);
            glm::vec3 m_globalLightColor = glm::vec3(1.0f, 1.0f, 1.0f);
            float m_globalLightIntensity = 30.0f;

            void configureViewport();
            void clearScreen();
        };
    } // namespace Render
} // namespace Engine