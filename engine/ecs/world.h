// // engine/ecs/world.h (Refatorado para OCP/SRP)

#pragma once

#include "entity.h"
#include "../../core/log.h"
#include "systems/base_system.h"
#include "i_component_manager.h"
#include "components/component_signature.h"

// Apenas para fins de definição de tipo.
#include "components/transform_component.h"
#include "components/mesh_component.h"
// ... inclua os outros componentes necessários aqui (movement, player, terrain, etc.) ...
#include "components/movement_component.h"
#include "components/player_component.h"
#include "components/terrain_component.h"
#include "components/terrain_tracker_component.h"
#include "components/camera_target_component.h"

#include <typeindex>
#include <utility>
#include <map>
#include <vector>
#include <memory>
#include <stdexcept>

namespace Engine
{
    namespace ECS
    {
        class World
        {
        private:
            EntityID m_nextEntityID = 1;

            // REMOVIDO: Todos os mapas de componentes específicos (e.g., m_movementComponents)

            // NOVO: Coleção de Gerenciadores de Componentes (Aplica OCP/SRP)
            // Key: ID do Tipo (std::type_index), Value: Abstração do Manager
            std::map<std::type_index, std::unique_ptr<IComponentManager>> m_componentManagers;

            // Vetor de sistemas (inalterado, pois atende LSP/SRP)
            std::vector<std::unique_ptr<System::BaseSystem>> m_systems;

            // --- NOVOS MEMBROS PARA O SISTEMA DE ASSINATURA ---
            // Mapeia o EntityID para sua Assinatura (quais componentes ela possui)
            std::map<EntityID, ComponentSignature> m_entitySignatures;

            // NOVO: Mapeia o tipo do System (type_index) para a Signature que ele requer.
            std::map<std::type_index, ComponentSignature> m_systemSignatures;

            // Mapeia o ID do Componente para o ID da Entidade (usado para otimizar busca, opcional)
            // std::map<ComponentTypeID, std::vector<EntityID>> m_componentEntityMap;

            // --- Mapeia o tipo T para o ID de componente (usa o TypeManager estático)
            template <typename T>
            ComponentTypeID getComponentTypeID()
            {
                // AQUI usamos o novo singleton para garantir um ID consistente.
                return ComponentTypeManager::Get().getTypeID<T>();
            }

            // --- Métodos de Acesso Privado aos Managers ---

            // Helper para obter o Manager tipado (versão não-const)
            template <typename T>
            ComponentManager<T> *getComponentManager()
            {
                std::type_index typeID = std::type_index(typeid(T));
                auto it = m_componentManagers.find(typeID);
                if (it == m_componentManagers.end())
                {
                    return nullptr; // Componente não registrado.
                }
                // Retorna o ponteiro para a implementação tipada
                return static_cast<ComponentManager<T> *>(it->second.get());
            }

            // Helper para obter o Manager tipado (versão const)
            template <typename T>
            const ComponentManager<T> *getComponentManager() const
            {
                std::type_index typeID = std::type_index(typeid(T));
                auto it = m_componentManagers.find(typeID);
                if (it == m_componentManagers.end())
                {
                    return nullptr;
                }
                // Retorna o ponteiro const para a implementação tipada
                return static_cast<const ComponentManager<T> *>(it->second.get());
            }

            // NOVO E CRÍTICO: LÓGICA DE MATCHING (A ser chamada em add/remove/destroy component)
            void checkSystemSignatures(EntityID entityID)
            {
                // Obtém a assinatura atual da entidade
                const ComponentSignature &entitySignature = m_entitySignatures.at(entityID);

                // Itera sobre todos os Systems no World
                for (const auto &systemPtr : m_systems)
                {

                    // Obtém o tipo do System (para buscar a signature requerida)
                    std::type_index systemType = std::type_index(typeid(*systemPtr));

                    // Verifica se o System tem uma Signature registrada
                    if (m_systemSignatures.count(systemType) == 0)
                    {
                        continue; // System não registrado.
                    }

                    const ComponentSignature &requiredSignature = m_systemSignatures.at(systemType);

                    // Lógica de Matching: Entidade DEVE conter TODOS os bits requeridos pelo System.
                    bool match = (entitySignature & requiredSignature) == requiredSignature;

                    // O ponteiro é um BaseSystem, mas queremos acessar m_entities, que agora
                    // está no BaseSystem.
                    System::BaseSystem *systemRawPtr = systemPtr.get();

                    if (match)
                    {
                        // Se corresponder, adiciona ao pool.
                        systemRawPtr->m_entities.insert(entityID);
                    }
                    else
                    {
                        // Se não corresponder, remove do pool.
                        systemRawPtr->m_entities.erase(entityID);
                    }
                }
            }

        public:
            World() = default;

            // 1. Cria uma nova Entity.
            EntityID createEntity()
            {
                EntityID id = m_nextEntityID++;
                // CRÍTICO: Inicializa a assinatura da nova entidade como vazia.
                m_entitySignatures[id] = ComponentSignature();
                return id;
            }

            // 2. REGISTRO DE COMPONENTES (Ponto de Extensão - OCP)
            template <typename T>
            void registerComponent()
            {
                std::type_index typeID = std::type_index(typeid(T));

                // CRÍTICO: Força o ComponentTypeManager a registrar o tipo T e assinalar um bit.
                getComponentTypeID<T>();

                if (m_componentManagers.count(typeID) == 0)
                {
                    m_componentManagers[typeID] = std::make_unique<ComponentManager<T>>();
                }
                else
                {
                    Engine::Core::Log::Warn("Componente ja registrado. Ignorando: " + std::string(typeid(T).name()));
                }
            }

            // 3. ADICIONAR COMPONENTES (Genérico - OCP-Compliant)
            template <typename T, typename... Args>
            void addComponent(EntityID entityID, Args &&...args)
            {
                if (auto manager = getComponentManager<T>())
                {
                    manager->addData(entityID, std::forward<Args>(args)...);

                    // --- ATUALIZAÇÃO DA ASSINATURA (O Coração da Mudança) ---
                    // 1. Obtém o ID do bit para o tipo T
                    ComponentTypeID typeBit = getComponentTypeID<T>();

                    // 2. Seta o bit na assinatura da entidade
                    m_entitySignatures.at(entityID).set(typeBit);

                    // 3. Notifica os sistemas (Próximo passo)
                    checkSystemSignatures(entityID);
                    // ---------------------------------------------------------
                }
                else
                {
                    Engine::Core::Log::Critical("Tentativa de adicionar um Componente nao registrado: " + std::string(typeid(T).name()) + ". Encerrando.");
                    std::terminate();
                }
            }
            
            // 4. REMOVER COMPONENTES (Novo, mas fundamental para a integridade do ECS)
            template <typename T>
            void removeComponent(EntityID entityID)
            {
                if (auto manager = getComponentManager<T>())
                {
                    manager->destroyComponent(entityID);

                    // --- ATUALIZAÇÃO DA ASSINATURA ---
                    ComponentTypeID typeBit = getComponentTypeID<T>();
                    // Zera o bit na assinatura da entidade
                    m_entitySignatures.at(entityID).reset(typeBit);

                    // 3. Notifica os sistemas (Próximo passo)
                    checkSystemSignatures(entityID);
                    // --------------------------------
                }
                else
                {
                    Engine::Core::Log::Warning("Tentativa de remover um Componente nao registrado: " + std::string(typeid(T).name()));
                }
            }

            // 4. OBTER COMPONENTES (Genérico - OCP-Compliant)
            template <typename T>
            T &getComponent(EntityID entityID)
            {
                if (auto manager = getComponentManager<T>())
                {
                    return manager->getData(entityID);
                }

                Engine::Core::Log::Critical("Tentativa de obter um Componente nao registrado ou nao mapeado: " + std::string(typeid(T).name()) + ". Encerrando.");
                // Retornar uma exceção é mais seguro que std::terminate, mas o terminate era o padrão anterior.
                throw std::runtime_error("Componente nao mapeado/registrado.");
            }

            // 5. OBTER TODOS OS COMPONENTES POR TIPO (Genérico para Iteração de Systems - OCP-Compliant)
            template <typename T>
            auto &getComponents()
            {
                if (auto manager = getComponentManager<T>())
                {
                    return manager->getAllData();
                }

                Engine::Core::Log::Critical("Tentativa de iterar sobre um Componente nao registrado ou nao mapeado: " + std::string(typeid(T).name()) + ". Encerrando.");
                throw std::runtime_error("Componente nao mapeado/registrado para iteracao.");
            }

            // 6. HAS COMPONENT (Genérico - OCP-Compliant)
            template <typename T>
            bool hasComponent(EntityID entityID) const
            {
                if (auto manager = getComponentManager<T>())
                {
                    return manager->hasData(entityID);
                }
                return false;
            }

            // 7. OBTER ENTIDADE ÚNICA (Adaptado para o ComponentManager)
            // OBS: Este método é uma utilidade para pegar EntityID de componentes únicos (e.g., Player, Terrain).
            template <typename T>
            EntityID getSingleEntityWith() const
            {
                if (auto manager = getComponentManager<T>())
                {
                    const auto &allData = manager->getAllData();
                    if (!allData.empty())
                    {
                        // Retorna o ID da primeira entidade no mapa (assumindo que há apenas uma)
                        return allData.begin()->first;
                    }
                }

                return INVALID_ENTITY_ID;
            }

            // 8. Destruição da Entidade (Agora também limpa a Assinatura)
            void destroyEntity(EntityID entityID)
            {
                // Limpa todos os componentes
                for (auto const &[type, manager] : m_componentManagers)
                {
                    manager->destroyComponent(entityID);
                }

                // Limpa a Assinatura (CRÍTICO)
                m_entitySignatures.erase(entityID);

                // Opcional: Notificar sistemas para remover a entidade de seus pools (checkSystemSignatures)
            }

            // NOVO: Registra a Assinatura Requerida para um Sistema
            template <typename T>
            void registerSystemSignature(ComponentSignature signature) {
                std::type_index systemType = std::type_index(typeid(T));
                
                // 1. Armazena a nova assinatura
                m_systemSignatures[systemType] = signature;
                
                // 2. CRÍTICO: Back-fill - Itera sobre todas as entidades existentes 
                // para preencher o pool m_entities do novo sistema.
                for (auto const& [entityID, entitySignature] : m_entitySignatures) {
                    // Chamar a checagem (que itera sobre todos os sistemas e agora inclui T)
                    checkSystemSignatures(entityID); 
                }
            }

            // 9. ADICIONAR MESH (Removido: Use addComponent<Component::Mesh> em seu lugar)
            // REMOVIDO: void addMesh(EntityID entity, uint32_t assetID) { ... }

            // MÉTODOS DE SISTEMA (inalterados)
            template <typename T, typename... Args>
            void addSystem(Args &&...args)
            {
                static_assert(std::is_base_of<System::BaseSystem, T>::value, "O sistema deve herdar de BaseSystem.");
                T *raw_system_ptr = new T(std::forward<Args>(args)...);
                std::unique_ptr<T> system_ptr(raw_system_ptr);
                m_systems.push_back(std::move(system_ptr));
            }

            template <typename T>
            T *getSystem()
            {
                for (const auto &systemPtr : m_systems)
                {
                    if (T *system = dynamic_cast<T *>(systemPtr.get()))
                    {
                        return system;
                    }
                }
                return nullptr;
            }

            void update(float dt)
            {
                for (const auto &system : m_systems)
                {
                    system->update(*this, dt);
                }
            }
        };

    } // namespace ECS
} // namespace Engine