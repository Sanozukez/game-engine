// // engine/ecs/systems/render_system.h (CORRIGIDO: Apenas Declarações)

#pragma once

#include "base_system.h"
#include "../components/transform_component.h"
#include "../../render/renderer.h" // Mantemos o include para o compilador do RenderSystem.cpp
#include "../components/animation_component.h"

namespace Engine
{
    namespace ECS
    {
        namespace System
        {

            class RenderSystem : public BaseSystem
            {
            private:
                Engine::Render::Renderer &m_renderer;

            public:
                // O construtor AGORA aceita a referência L-VALUE do wrapper
                RenderSystem(Engine::Render::Renderer &renderer);                 

                // Remova TODO o corpo {...} daqui. Mantenha apenas a declaração.
                void update(World &world, float dt) override;
            };

        } // namespace System
    } // namespace ECS
} // namespace Engine