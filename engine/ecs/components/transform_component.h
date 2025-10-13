// // engine/ecs/components/transform_component.h

#pragma once

#include "../../math/vec3.h"
#include "../../math/quat.h"

namespace Engine {
namespace ECS {
namespace Component { // Namespace dedicado para componentes (SRP!)

// TransformComponent: Dados puros de posição, rotação e escala.
struct Transform {
    Engine::Math::Vec3 position;
    Engine::Math::Quat rotation;
    Engine::Math::Vec3 scale;

    // Construtor Default: Garante a inicialização limpa (apenas dados)
    Transform() : position(0.0f), rotation(Engine::Math::Quat()), scale(1.0f) {}

   

    // Nenhuma função de "update" ou "render" aqui. É apenas DATA.
};

} // namespace Component
} // namespace ECS
} // namespace Engine