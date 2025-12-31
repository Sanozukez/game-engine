// // engine/ecs/systems/render_system.h

#pragma once

#include "ecs/systems/base_system.h"
#include "client/render/renderer.h"
#include "client/render/armature_renderer.h" // Incluído para a instância
#include "engine/asset/asset_manager.h"      // Necessário para o membro m_assetManager
#include "../components/transform_component.h"
#include "../components/animation_component.h" // Para usar Component::Animation
#include "../components/mesh_component.h"      // Para usar Component::Mesh

namespace Engine
{
    namespace ECS
    {
        namespace System
        {

            class RenderSystem : public BaseSystem
            {
            public:
                RenderSystem(Engine::Render::Renderer &renderer);

                void update(World &world, float dt) override;

            private:
                Engine::Render::Renderer &m_renderer;

                // NOVO: Referência ao AssetManager (Singleton)
                Engine::Asset::AssetManager &m_assetManager;

                // NOVO: Instância do Armature Renderer (movido para private - SRP)
                Engine::Render::ArmatureRenderer m_armatureRenderer;
            };

        } // namespace System
    } // namespace ECS
} // namespace Engine
