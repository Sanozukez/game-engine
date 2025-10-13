// // engine/ecs/world_loader.h

#pragma once

#include "../ecs/entity.h" // Para EntityID
#include <string>

// Forward Declaration
namespace Engine::ECS {
    class World;
}

namespace Engine {
namespace ECS {

class WorldLoader {
public:
    // Método estático para carregar o mapa (Scene) e popular o World com Entities.
    // static bool Load(World& world);
    static bool Load(World& world, EntityID playerID);
};

} // namespace ECS
} // namespace Engine
