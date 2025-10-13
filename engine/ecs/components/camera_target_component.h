// engine/ECS/components/camera_target_component.h
#pragma once

namespace Engine {
namespace ECS {
namespace Component {

// Adicionado à Entidade Player. Define as propriedades do alvo que a câmera deve rastrear.
struct CameraTarget {
    // Offset vertical (altura dos olhos/foco) da entidade.
    float focusHeight = 1.0f; 
    
    // NOTA: A posição X/Y/Z é lida diretamente do TransformComponent da entidade.
};

} // namespace Component
} // namespace ECS
} // namespace Engine