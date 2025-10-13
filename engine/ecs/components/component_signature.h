// // engine/ecs/component_signature.h (Novo arquivo)

#pragma once

#include <bitset>
#include <cstdint>
#include <stdexcept>
#include <map>
#include <typeindex> // Para TypeID

namespace Engine {
namespace ECS {

    // Define o número máximo de tipos de componentes que a engine pode ter.
    // 64 bits é o limite superior para bitset eficiente.
    const size_t MAX_COMPONENTS = 64; 

    // O Bitset que representa o conjunto de componentes que uma Entidade possui
    // ou que um Sistema requer. (A Assinatura)
    using ComponentSignature = std::bitset<MAX_COMPONENTS>;
    
    // Tipo para indexar os Componentes no mapeamento global.
    using ComponentTypeID = std::size_t; 


    // --- Gerenciador Global de Componentes (SRP/SRP) ---
    // Esta classe tem a ÚNICA responsabilidade de dar IDs para tipos de Componentes.
    class ComponentTypeManager {
    private:
        // Mapeia o tipo (std::type_index) para um ID sequencial.
        std::map<std::type_index, ComponentTypeID> m_componentTypes;
        ComponentTypeID m_nextTypeID = 0;

    public:
        // Obtém a referência estática e única.
        static ComponentTypeManager& Get() {
            static ComponentTypeManager instance;
            return instance;
        }

        // Método genérico para obter o ID de um tipo T (Cria se não existir)
        template <typename T>
        ComponentTypeID getTypeID() {
            std::type_index typeID = std::type_index(typeid(T));

            // Se o tipo já foi registrado, retorna o ID existente.
            if (m_componentTypes.count(typeID)) {
                return m_componentTypes[typeID];
            }

            // CRÍTICO: Registra o novo tipo e verifica o limite.
            if (m_nextTypeID >= MAX_COMPONENTS) {
                // Se esse erro ocorrer, você precisa aumentar MAX_COMPONENTS.
                throw std::runtime_error("Limite maximo de componentes excedido! Aumente MAX_COMPONENTS.");
            }

            // Assinala o próximo ID, incrementa e retorna.
            m_componentTypes[typeID] = m_nextTypeID;
            return m_nextTypeID++;
        }

        // Impede cópia e atribuição (Singleton Pattern)
        ComponentTypeManager(const ComponentTypeManager&) = delete;
        ComponentTypeManager& operator=(const ComponentTypeManager&) = delete;

    private:
        ComponentTypeManager() = default;
    };


} // namespace ECS
} // namespace Engine