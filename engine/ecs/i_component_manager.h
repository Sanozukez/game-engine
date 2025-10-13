// // engine/ecs/i_component_manager.h (Implementação SRP/OCP/LSP)

#pragma once

#include "entity.h" // Para EntityID
#include <map>
#include <memory>
#include <stdexcept>

namespace Engine {
namespace ECS {

// 1. Interface Base IComponentManager: O Contrato (Aplica LSP/DIP)
// O World depende desta abstração, não das implementações concretas de componentes.
class IComponentManager {
public:
    virtual ~IComponentManager() = default;
    
    // Método não-tipado para remover um componente quando a entidade é destruída.
    virtual void destroyComponent(EntityID entityID) = 0;
};

// 2. Implementação Concreta ComponentManager<T>: O Gerenciador de Tipos Específicos (Aplica SRP)
// Esta classe é a única responsável por armazenar e gerenciar um ÚNICO tipo de Componente T.
template <typename T>
class ComponentManager : public IComponentManager {
private:
    // O mapa armazena os dados do Componente T, indexados pelo ID da Entidade.
    std::map<EntityID, T> m_componentData;
    
public:
    // Adiciona ou substitui o componente T para a entidade
    template <typename... Args>
    void addData(EntityID entityID, Args&&... args) {
        // Usa `emplace` para construção in-place e evita cópias desnecessárias
        m_componentData.emplace(entityID, T(std::forward<Args>(args)...));
    }
    
    // Obtém o componente T (referência mutável)
    T& getData(EntityID entityID) { 
        // Lança std::out_of_range se o componente não existir (comportamento de 'at')
        return m_componentData.at(entityID); 
    }
    
    // Obtém o mapa inteiro (para iteração de Systems)
    // O sistema de ECS itera sobre o mapa do manager.
    std::map<EntityID, T>& getAllData() {
        return m_componentData;
    }
    
    // Obtém o mapa inteiro (versão const)
    const std::map<EntityID, T>& getAllData() const {
        return m_componentData;
    }


    // Verifica se a entidade possui o componente T
    bool hasData(EntityID entityID) const {
        return m_componentData.count(entityID) > 0;
    }
    
    // Remove o componente T (Implementação do Contrato Base)
    void destroyComponent(EntityID entityID) override {
        m_componentData.erase(entityID);
    }
};

} // namespace ECS
} // namespace Engine