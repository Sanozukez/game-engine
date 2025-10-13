// // engine/ecs/world.h

#pragma once

#include "entity.h"
#include "../../core/log.h" // Para uso em getComponent (CRITICAL)
#include "components/transform_component.h"
#include "components/mesh_component.h"
#include "systems/base_system.h"
#include "components/movement_component.h"
#include "components/player_component.h"
#include "components/terrain_component.h" // <--- NOVO INCLUDE: Para o Raycasting
#include "components/terrain_tracker_component.h"
#include "components/camera_target_component.h"

#include <utility>
#include <map>
#include <vector>
#include <memory>
#include <type_traits> // Para std::is_same_v
#include <stdexcept>   // Para std::terminate

namespace Engine
{
    namespace ECS
    {
        // NOTA: A constante agora está definida no entity.h
        // const static EntityID INVALID_ENTITY_ID = 0;

        // World: A principal entidade que armazena todos os dados e sistemas.
        class World
        {
        private:
            EntityID m_nextEntityID = 1;

            // Mapeamento de Entidades para seus Componentes.
            std::map<EntityID, Component::Transform> m_transformComponents;
            std::map<EntityID, Component::Mesh> m_meshComponents;

            // Armazenamento para os novos Componentes de Gameplay
            std::map<EntityID, Component::Movement> m_movementComponents;
            std::map<EntityID, Component::Player> m_playerComponents;
            std::map<EntityID, Component::Terrain> m_terrainComponents; // <--- NOVO MAPA PARA TERRAIN
            std::map<EntityID, Component::TerrainTracker> m_terrainTrackerComponents;
            std::map<EntityID, Component::CameraTarget> m_cameraTargetComponents;

            // Vetor de sistemas (para o loop de update)
            std::vector<std::unique_ptr<System::BaseSystem>> m_systems;

        public:
            World() = default;

            // 1. Cria uma nova Entity, retornando seu ID.
            EntityID createEntity()
            {
                EntityID id = m_nextEntityID++;
                // Uma entidade deve ter pelo menos um componente base (Ex: Transform)
                m_transformComponents[id] = Component::Transform();
                return id;
            }

            // 2. OBTER TRANSFORM (Atalho)
            Component::Transform &getTransform(EntityID entity)
            {
                return m_transformComponents.at(entity);
            }

            const std::map<EntityID, Component::Transform> &getTransformComponents() const
            {
                return m_transformComponents;
            }

            // NOVO: Adiciona a verificação genérica (Usado pelo RenderSystem)
            template <typename T>
            bool hasComponent(EntityID entityID) const
            {
                if constexpr (std::is_same_v<T, Component::Mesh>)
                {
                    return m_meshComponents.count(entityID);
                }
                // Adicionar outras condições aqui (e.g., Movement, Player, etc.)
                return false;
            }
            // ---------------------------------------------

            // 3. ADICIONAR COMPONENTES (Template Variádico)
            template <typename T, typename... Args>
            void addComponent(EntityID entityID, Args &&...args)
            {
                // Usamos a trait 'if constexpr' do C++17 para rotear a inserção
                if constexpr (std::is_same_v<T, Component::Movement>)
                {
                    m_movementComponents[entityID] = T(std::forward<Args>(args)...);
                }
                else if constexpr (std::is_same_v<T, Component::Player>)
                {
                    m_playerComponents[entityID] = T(std::forward<Args>(args)...);
                }
                else if constexpr (std::is_same_v<T, Component::Terrain>) // <--- ROTA PARA TERRAIN
                {
                    m_terrainComponents[entityID] = T(std::forward<Args>(args)...);
                }
                else if constexpr (std::is_same_v<T, Component::TerrainTracker>)
                {
                    m_terrainTrackerComponents[entityID] = T(std::forward<Args>(args)...);
                }
                // ... (Mais Componentes podem ser adicionados aqui) ...
            }

            // 4. OBTER COMPONENTES (Template Genérico)
            template <typename T>
            T &getComponent(EntityID entityID)
            {
                // NOTA: O RenderSystem agora deve usar getComponent<Component::Mesh>(entityID)
                if constexpr (std::is_same_v<T, Component::Movement>)
                {
                    return m_movementComponents.at(entityID);
                }
                else if constexpr (std::is_same_v<T, Component::Transform>)
                {
                    return m_transformComponents.at(entityID);
                }
                else if constexpr (std::is_same_v<T, Component::Mesh>) // <--- ROTA PARA MESH
                {
                    return m_meshComponents.at(entityID);
                }
                else if constexpr (std::is_same_v<T, Component::Terrain>) // <--- ROTA PARA TERRAIN
                {
                    return m_terrainComponents.at(entityID);
                }
                else if constexpr (std::is_same_v<T, Component::TerrainTracker>)
                {
                    return m_terrainTrackerComponents.at(entityID);
                }
                else if constexpr (std::is_same_v<T, Component::Terrain>)
                {
                    return m_terrainComponents.at(entityID);
                }
                else if constexpr (std::is_same_v<T, Component::CameraTarget>)
                {
                    return m_cameraTargetComponents.at(entityID);
                }
                else
                {
                    Engine::Log::Critical("Tentativa de obter um Componente nao mapeado. Encerrando.");
                    std::terminate();
                }
            }
            // ---------------------------------------------

            // 5. OBTER TODOS OS COMPONENTES POR TIPO (Template Genérico para Iteração de Systems)
            template <typename T>
            auto &getComponents()
            {
                if constexpr (std::is_same_v<T, Component::TerrainTracker>)
                {
                    return m_terrainTrackerComponents;
                }
                // NOVO: Rota para CameraTarget
                else if constexpr (std::is_same_v<T, Component::CameraTarget>) // <--- CORREÇÃO DO C7683
                {
                    return m_cameraTargetComponents;
                }
                // Adicionar mais verificações para Mesh, Movement, etc., se necessário.
                else if constexpr (std::is_same_v<T, Component::Transform>)
                {
                    return m_transformComponents;
                }
                else
                {
                    // Se a rota falhar, o compilador tenta retornar algo que gera o erro C7683/C3313
                    Engine::Log::Critical("Tentativa de iterar sobre um Componente nao mapeado. Encerrando.");
                    std::terminate();
                }
            }

            // 6. OBTER ENTIDADE ÚNICA (Resolve C2672 do PlayerSystem)
            template <typename T>
            EntityID getSingleEntityWith() const
            {
                if constexpr (std::is_same_v<T, Component::Player>)
                {
                    if (!m_playerComponents.empty())
                    {
                        return m_playerComponents.begin()->first;
                    }
                }
                else if constexpr (std::is_same_v<T, Component::Terrain>) // <--- ROTA PARA TERRAIN
                {
                    if (!m_terrainComponents.empty())
                    {
                        return m_terrainComponents.begin()->first;
                    }
                }
                // Retorna 0 (INVALID_ENTITY_ID) se não for encontrado ou não for mapeado.
                return INVALID_ENTITY_ID;
            }

            // MÉTODO CRUCIAL: Adicionar um Sistema (Solução que funcionou para referências)
            template <typename T, typename... Args>
            void addSystem(Args &&...args)
            { // <--- Esta é a função que será chamada no app.cpp!
                static_assert(std::is_base_of<System::BaseSystem, T>::value, "O sistema deve herdar de BaseSystem.");

                // Usa 'new' para contornar problemas de MSVC com make_unique e referências
                T *raw_system_ptr = new T(std::forward<Args>(args)...);
                std::unique_ptr<T> system_ptr(raw_system_ptr);
                m_systems.push_back(std::move(system_ptr));
            }

            // 7. OBTER SISTEMA POR TIPO
            template <typename T>
            T *getSystem() // <--- AGORA RETORNA UM PONTEIRO!
            {
                for (const auto &systemPtr : m_systems)
                {
                    if (T *system = dynamic_cast<T *>(systemPtr.get()))
                    {
                        return system; // Retorna o ponteiro
                    }
                }
                // Se não for encontrado, retorna nullptr (seguro)
                return nullptr; // <--- SEGURO, ELIMINA std::terminate
            }

            // 8. ADICIONAR MESH (Simplificado)
            void addMesh(EntityID entity, uint32_t assetID)
            {
                m_meshComponents[entity] = Component::Mesh(assetID);
            }

            // 9. Método principal para rodar todos os sistemas
            void update(float dt)
            {
                for (const auto &system : m_systems)
                {
                    system->update(*this, dt);
                }
            }

            // REMOVIDOS os métodos addMesh, getMeshComponent, hasMeshComponent antigos
            // Eles foram substituídos pelos templates acima.
        };

    } // namespace ECS
} // namespace Engine
