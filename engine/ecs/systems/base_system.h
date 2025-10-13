// // engine/ecs/systems/base_system.h (Adaptado para Assinaturas)

#pragma once

#include "../../ecs/entity.h"
#include "../../ecs/components/component_signature.h" // NOVO: Inclui ComponentSignature

#include <vector>
#include <set> // Usaremos um set ou vector para armazenar as entidades. Um set garante unicidade e é mais rápido para remoção/adição.
#include <memory>

// FORWARD DECLARATION de World e Renderer
namespace Engine {
    namespace ECS {
        class World; 
    }
    namespace Render {
        class Renderer; 
    }
}

namespace Engine {
namespace ECS {
namespace System { 

class BaseSystem { 
public:
    BaseSystem() = default;
    virtual ~BaseSystem() = default;

    // Lista de entidades que o World atribuiu a este System (DIP)
    std::set<EntityID> m_entities; 

    virtual void update(ECS::World& world, float dt) = 0; 
};

// --- NOVO: Template Base para Definir a Assinatura (DIP e OCP) ---
// Todo System concreto deve herdar desta classe.
template <typename T>
class System : public BaseSystem {
public:
    // Lista de entidades que possuem a ComponentSignature necessária.
    // O World será responsável por gerenciar esta lista.
    std::set<EntityID> m_entities; 

    // NOVO: Método para Obter a Assinatura Requerida pelo Sistema.
    // É uma boa prática definir isso no System, mas o World precisa ter acesso.
    // Vamos usar a herança CRTP (Curiously Recurring Template Pattern) para isso.

    // Método que será usado no World para registrar a assinatura de T.
    static ComponentSignature getRequiredSignature() {
        ComponentSignature signature;
        // O System deve implementar um método estático T::defineSignature()
        // ou a chamada a T::defineSignature() deve ser feita no AppSetup após o addSystem.
        
        // CRÍTICO: Para manter o DIP, o World não deve saber de todos os Systems.
        // O World só precisa de um mapa de Signature -> EntityID. 
        // O System, por sua vez, armazena o set<EntityID> m_entities.

        // Por enquanto, vamos manter este método como placeholder e definir a Signature
        // em um novo Manager para evitar que o BaseSystem dependa de todos os componentes.
        return signature;
    }
    
    // Simplificando: o World saberá que T é um System e terá um mapa de Systems.
    // A assinatura do System será gerenciada pelo World para que ele possa checar o match.
};

} // namespace System
} // namespace ECS
} // namespace Engine