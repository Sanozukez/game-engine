// // engine/ecs/components/movement_component.h
#pragma once

#include "../../math/vec3.h" // Para TargetDestination

namespace Engine {
namespace ECS {
namespace Component {

// Captura todas as variáveis de estado e controle de movimento do PlayerCharacter
struct Movement {
    // Dados de Velocidade (m_movementSpeed, m_rotationSpeed)
    float movementSpeed = 5.0f;
    float rotationSpeed = 75.0f; // Graus por segundo (usado no Bloco 3 do código antigo)

    // Dados de Estado (m_isMovingToDestination, m_isCameraOrbitModeActive)
    bool isMovingToDestination = false;
    
    // Este estado é fundamental para a lógica de input do PlayerCharacter
    bool isCameraOrbitModeActive = false; 

    // Dados de Destino (m_targetDestination)
    Engine::Math::Vec3 targetDestination{0.0f, 0.0f, 0.0f};

    // Dados de Visualização (m_cameraFocusHeight)
    float cameraFocusHeight = 1.0f;

     // NOVO: Velocidade atual (para interpolação e aceleração/desaceleração)
    float currentVelocity = 0.0f; 
    
    // NOVO: Taxa de Aceleração/Desaceleração
    float accelerationRate = 15.0f; // Ex: 15 metros/s² (Ajuste conforme o game feel)
    
    // NOTA DE ARQUITETURA:
    // O TransformComponent (position, rotation) já lida com a posição.
    // Este componente lida com o COMO o objeto se move.
};

} // namespace Component
} // namespace ECS
} // namespace Engine