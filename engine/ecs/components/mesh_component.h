// // engine/ecs/components/mesh_component.h

#pragma once

#include <cstdint>
#include <memory>
// Forward declaration para evitar include pesado (melhor acoplamento)
namespace Engine::Asset
{
    class Model;
}

namespace Engine
{
    namespace ECS
    {
        namespace Component
        {

            // MeshComponent: Dados puros sobre o asset 3D e sua referência.
            struct Mesh
            {
                // Referência ao AssetID carregado pelo AssetManager (MMAP)
                uint32_t assetID;

                // O AssetManager fará o cache, mas o Componente pode ter um ponteiro leve
                // para o modelo real (depende do design do seu Renderer).
                // Por enquanto, apenas o ID é suficiente para o RenderSystem.

                // FUTURE: std::shared_ptr<Engine::Asset::Model> cachedModel;

                Mesh(uint32_t id = 0) : assetID(id) {}
            };

        } // namespace Component
    } // namespace ECS
} // // namespace Engine