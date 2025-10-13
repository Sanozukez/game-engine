#pragma once

#include "../../math/vec3.h"

namespace Engine {
namespace ECS {
namespace Component {

// Adiciona esta flag à entidade Player. Indica que ela deve ser rastreada
// para colisões de terreno (ajuste de altura).
struct TerrainTracker {
    // A altura y do chão será armazenada aqui após o raycast.
    float groundHeight = 0.0f;
    
    // NOTA: cameraFocusHeight permanece no MovementComponent.
};

} // namespace Component
} // namespace ECS
} // namespace Engine