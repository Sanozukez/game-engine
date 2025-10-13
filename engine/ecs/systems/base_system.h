// // engine/ecs/systems/base_system.h (CORREÇÃO DE DEFINIÇÃO)

#pragma once

// FORWARD DECLARATION de World (para evitar include pesado)
namespace Engine {
    namespace ECS {
        class World; 
    }
}
// FORWARD DECLARATION de Renderer (embora não seja necessário para BaseSystem, evita erros em cadeia)
namespace Engine {
    namespace Render {
        class Renderer; 
    }
}

namespace Engine {
namespace ECS {
namespace System { 

class BaseSystem { // <--- A classe base começa aqui!
public:
    BaseSystem() = default;
    virtual ~BaseSystem() = default;

    // Garante que o World e o dt sejam a única responsabilidade
    virtual void update(ECS::World& world, float dt) = 0; 
};

} // namespace System
} // namespace ECS
} // namespace Engine