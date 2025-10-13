// // engine/ecs/entity.h

#pragma once

#include <cstdint>

namespace Engine
{
    namespace ECS
    {

        // Usamos um tipo simples para representar a Entidade (apenas a identidade)
        // Um uint64_t fornece IDs suficientes para qualquer escopo de Engine
        using EntityID = uint64_t;

        const EntityID INVALID_ENTITY_ID = 0; // <--- ADICIONAR ESTA LINHA!


        // O Entity Manager (que estará no World) gerenciará a atribuição e reutilização desses IDs.

    } // namespace ECS
} // namespace Engine