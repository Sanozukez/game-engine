#pragma once

#include "base_system.h"
#include <limits> // Para std::numeric_limits
#include <optional> // <--- CORREÇÃO 1: Incluir o header para std::optional
#include <glm/glm.hpp> // <--- CORREÇÃO 2: Incluir o header para glm::vec3
#include <glm/gtc/quaternion.hpp> // <--- CORREÇÃO 3: Geralmente necessário para as funções GLM

// Forward declarations dos Componentes e dependências
namespace Engine::ECS::Component {
    struct Transform;
    struct Terrain;
    struct Mesh;
    struct TerrainTracker; // O novo componente
}
namespace Engine::Asset {
    class AssetManager;
    class Model;
}
namespace Engine::Physics {
std::optional<float> rayTriangleIntersect(const glm::vec3 &rayOrigin, const glm::vec3 &rayDirection, const glm::vec3 &v0, const glm::vec3 &v1, const glm::vec3 &v2);}

namespace Engine {
namespace ECS {
namespace System {

class TerrainTrackingSystem : public BaseSystem {
public:
    TerrainTrackingSystem() = default;

    // A lógica complexa do Raycasting do Bloco 6 do PlayerSystem é migrada para cá.
    void update(World &world, float dt) override;
};

} // namespace System
} // namespace ECS
} // namespace Engine