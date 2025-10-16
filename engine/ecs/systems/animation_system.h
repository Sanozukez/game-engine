// // engine/ecs/systems/animation_system.h
#pragma once

#include "base_system.h"
#include "../../asset/asset_manager.h" // Dependência para buscar dados de animação

// Forward declarations dos Componentes
namespace Engine::ECS::Component
{
    struct Animation;
    struct Movement;
    struct Transform;
}

namespace Engine
{
    namespace ECS
    {
        namespace System
        {
            // O AnimationSystem tem a responsabilidade única de traduzir o estado de movimento 
            // em transformações de ossos (Bones).
            class AnimationSystem : public BaseSystem
            {
            private:
                Engine::Asset::AssetManager& m_assetManager;

            public:
                // O construtor injeta o AssetManager (DIP)
                AnimationSystem(Engine::Asset::AssetManager& assetManager);
                
                // O update executa a lógica de cálculo de frames e blend.
                void update(World &world, float dt) override;
                
                // Utilitário para converter string de nome da animação em Hash ID
                uint32_t getAnimationID(const std::string& animationName) const;
            };

        } // namespace System
    } // namespace ECS
} // namespace Engine