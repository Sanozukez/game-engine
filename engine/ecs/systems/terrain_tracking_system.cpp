#include "terrain_tracking_system.h"
#include "../../ecs/world.h"
#include "../../ecs/components/transform_component.h"
#include "../../ecs/components/terrain_component.h"
#include "../../ecs/components/mesh_component.h"
#include "../../ecs/components/terrain_tracker_component.h"
#include "../../asset/asset_manager.h"
#include "../../asset/model.h"
#include "../../physics/raycaster.h" // Assumindo que rayTriangleIntersect está aqui

#include <glm/glm.hpp>

namespace Engine
{
    namespace ECS
    {
        namespace System
        {

            void TerrainTrackingSystem::update(World &world, float dt)
            {
                // 1. Encontrar a entidade de Terreno (fonte de dados para o Raycasting)
                const EntityID terrainID = world.getSingleEntityWith<Component::Terrain>();

                if (terrainID == INVALID_ENTITY_ID)
                {
                    return;
                }

                Component::Mesh &terrainMesh = world.getComponent<Component::Mesh>(terrainID);
                std::shared_ptr<Asset::Model> terrainModel = Asset::AssetManager::Get().getModel(terrainMesh.assetID);

                if (!terrainModel)
                {
                    return;
                }

                // 2. Iterar sobre todas as entidades que precisam ser rastreadas (Player e outros)
                // NOVO: Use a forma tradicional de iteração que funciona em qualquer compilador C++17/20
                for (auto const &pair : world.getComponents<Component::TerrainTracker>())
                {
                    // Acessa EntityID e o Componente explicitamente
                    const EntityID entityID = pair.first;
                    // Note que 'tracker' não é usado neste sistema, mas a variável é necessária para a coerência do loop.
                    // const Component::TerrainTracker &tracker = pair.second;

                    // Certifique-se de que a entidade tem o TransformComponent
                    if (!world.hasComponent<Component::Transform>(entityID))
                        continue;

                    // Use getComponent com o tipo explícito
                    Component::Transform &transform = world.getComponent<Component::Transform>(entityID);

                    float focusHeight = 0.0f;
                    if (world.hasComponent<Component::Movement>(entityID))
                    {
                        focusHeight = world.getComponent<Component::Movement>(entityID).cameraFocusHeight;
                    }

                    // CÓDIGO MIGRADADO DO BLOCO 6 DO PLAYER SYSTEM
                    glm::vec3 rayOrigin = glm::vec3(transform.position.x, 
                                        100.0f, // Use 100.0f como base alta
                                        transform.position.z);
                    glm::vec3 rayDirection(0.0f, -1.0f, 0.0f);
                    float minDistance = std::numeric_limits<float>::max();
                    bool foundIntersection = false;

                    for (const auto &mesh_ptr : terrainModel->getMeshes())
                    {
                        // Certifique-se de que os tipos de retorno de getVertices/getIndices são consistentes
                        // e use const reference para evitar cópia desnecessária.

                        // CORREÇÃO: Usar auto para inferir o tipo, mas como cópia se for o caso do seu sistema
                        // Mudar para const auto& garante que o tipo é inferido corretamente e não há cópia.
                        const auto &vertices = mesh_ptr->getVertices();
                        const auto &indices = mesh_ptr->getIndices();

                        for (size_t i = 0; i < indices.size(); i += 3)
                        {
                            // O acesso por índice deve usar o mesmo tipo de retorno das coleções.
                            // Certifique-se de que mesh_ptr->getVertices() retorna um container acessível por [].

                            // Assumindo que vertices/indices são std::vector:
                            const glm::vec3 &v0 = vertices[indices[i]].Position;
                            const glm::vec3 &v1 = vertices[indices[i + 1]].Position;
                            const glm::vec3 &v2 = vertices[indices[i + 2]].Position;

                            // Assumimos que Engine::Physics::rayTriangleIntersect está globalmente acessível
                            if (auto distance = Engine::Physics::rayTriangleIntersect(rayOrigin, rayDirection, v0, v1, v2))
                            {
                                if (*distance < minDistance)
                                {
                                    minDistance = *distance;
                                    foundIntersection = true;
                                }
                            }
                        }
                    }

                    // 3. Aplicar o Ajuste de Altura (Atualiza o Componente Transform)
                    if (foundIntersection)
                    {
                        float groundHeight = rayOrigin.y - minDistance;

                        // --- CORREÇÃO AQUI ---
                        // Se o modelo tem o ponto pivot (origem) no chão (sola do pé),
                        // o offset é 0.0f.

                        // Comentamos (ou removemos) a lógica que adicionava o modelHeightOffset:
                        /*
                        float modelHeightOffset = 0.0f;
                        if (world.hasComponent<Component::Movement>(entityID))
                        {
                            modelHeightOffset = world.getComponent<Component::Movement>(entityID).cameraFocusHeight;
                        }
                        */

                        // Aplicar a correção: A posição Y do personagem é exatamente a altura do chão.
                        transform.position.y = groundHeight;

                        // NOTA: A variável cameraFocusHeight ainda é usada no PlayerSystem
                        // para o m_camera.setTarget(), o que está correto.
                    }
                }
            }

        } // namespace System
    } // namespace ECS
} // namespace Engine