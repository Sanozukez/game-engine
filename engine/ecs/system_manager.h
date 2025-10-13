// // engine/ecs/system_manager.h (Novo arquivo - Alto SRP)

#pragma once

#include "entity.h"
#include "systems/base_system.h"
#include "components/component_signature.h"

#include <map>
#include <memory>
#include <typeindex>
#include <stdexcept>

namespace Engine {
namespace ECS {

class SystemManager {
private:
    // Mapeia o tipo do System (RenderSystem, PlayerSystem, etc.) para sua Assinatura.
    std::map<std::type_index, ComponentSignature> m_signatures;
    
    // Mapeia o tipo do System para o ponteiro base, para fácil acesso/notificação.
    // O World ainda armazena o vetor de unique_ptr<BaseSystem>, mas este Manager
    // precisa saber quais systems existem.
    
    // O World será o "dono" do vetor de systems (m_systems). 
    // O SystemManager apenas gerencia as assinaturas e a lógica de correspondência.

public:
    // 1. Registra a assinatura que o System T precisa.
    template <typename T>
    void registerSignature(ComponentSignature signature) {
        m_signatures[std::type_index(typeid(T))] = signature;
    }

    // 2. Verifica e atualiza a lista de entidades de um System (O coração do ECS)
    template <typename T>
    void entitySignatureChanged(EntityID entityID, const ComponentSignature& entitySignature, T* systemPtr) {
        std::type_index systemType = std::type_index(typeid(T));
        
        // Verifica se o System T está registrado
        if (m_signatures.count(systemType) == 0) {
            return; // System não registrado.
        }

        // Obtém a assinatura que o System T requer.
        const ComponentSignature& requiredSignature = m_signatures.at(systemType);

        // Lógica de Correspondência (Matching Logic)
        // ----------------------------------------------------------------------
        // Se a assinatura da Entidade (AND) a assinatura requerida for igual 
        // à assinatura requerida, significa que a entidade possui TODOS os componentes
        // que o sistema T precisa. (requiredSignature & entitySignature) == requiredSignature
        
        bool match = (entitySignature & requiredSignature) == requiredSignature;

        if (match) {
            // Se corresponder, garante que a entidade está no pool do System.
            systemPtr->m_entities.insert(entityID);
        } else {
            // Se não corresponder, garante que a entidade NÃO está no pool.
            systemPtr->m_entities.erase(entityID);
        }
        // ----------------------------------------------------------------------
    }
    
    // 3. Remove a entidade de todos os systems quando ela é destruída.
    void removeEntityFromAllSystems(EntityID entityID, const std::vector<std::unique_ptr<System::BaseSystem>>& systems) {
        for (const auto& systemPtr : systems) {
            // CRÍTICO: Precisamos de um cast para o System<T> concreto para acessar m_entities.
            // Isso geralmente é resolvido refatorando o World para gerenciar o SystemManager
            // de forma mais tipada, mas por enquanto, fazemos um cast base.
            // Para simplificar, vou assumir que o m_entities foi movido para BaseSystem 
            // (como no código que vou propor) para evitar o dynamic_cast repetitivo aqui.
            // Vamos mudar a abordagem do BaseSystem.
        }
    }

};

} // namespace ECS
} // namespace Engine