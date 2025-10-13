// engine/ECS/components/camera_input_component.h
#pragma once

#include <glm/glm.hpp>

namespace Engine {
namespace ECS {
namespace Component {

// Adicionado ao Player. Armazena os vetores que o PlayerSystem consome para movimento.
struct CameraInput {
    glm::vec3 forward = glm::vec3(0.0f, 0.0f, 1.0f);
    glm::vec3 right = glm::vec3(1.0f, 0.0f, 0.0f);
    float yaw_degrees = 0.0f; // Yaw da Câmera (em graus)
};

} // namespace Component
} // namespace ECS
} // namespace Engine