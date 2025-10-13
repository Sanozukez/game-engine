#include "terrain_tracking_system.h"
#include "../../ecs/world.h"
#include "../../ecs/components/transform_component.h"
#include "../../ecs/components/terrain_component.h"
#include "../../ecs/components/mesh_component.h"
#include "../../ecs/components/terrain_tracker_component.h"
#include "../../ecs/components/movement_component.h"
#include "../../asset/asset_manager.h"
#include "../../asset/model.h"
#include "../../physics/raycaster.h"

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

                // NOVO: Obtém o Mesh do Terrain (Componente estático)
                Component::Mesh &terrainMesh = world.getComponent<Component::Mesh>(terrainID);

                std::shared_ptr<Asset::Model> terrainModel = Asset::AssetManager::Get().getModel(terrainMesh.assetID);

                if (!terrainModel)
                {
                    // Se o modelo não carregar, sai do System.
                    return;
                }

                // 2. Itera sobre o pool de entidades (m_entities) que precisam de tracking.
                // Requer: Transform e TerrainTracker (conforme registrado no AppSetup).
                for (const EntityID entityID : m_entities)
                {
                    Component::Transform &transform = world.getComponent<Component::Transform>(entityID);
                    Component::TerrainTracker &tracker = world.getComponent<Component::TerrainTracker>(entityID);

                    std::shared_ptr<Asset::Model> terrainModel = Asset::AssetManager::Get().getModel(terrainMesh.assetID);

                    if (!terrainModel)
                    {
                        return;
                    }

                    // 2. Itera sobre o pool de entidades (m_entities) que precisam de tracking. (DIP)
                    // A Signature garante que todas as entidades aqui têm Transform e TerrainTracker.
                    for (const EntityID entityID : m_entities)
                    {
                        // Obtém os Componentes necessários (sem checagem, World garante)
                        Component::Transform &transform = world.getComponent<Component::Transform>(entityID);
                        // Component::TerrainTracker &tracker = world.getComponent<Component::TerrainTracker>(entityID); // Não é usada diretamente

                        // Lógica para obter a altura de foco, se houver Movement
                        float focusHeight = 0.0f;
                        if (world.hasComponent<Component::Movement>(entityID))
                        {
                            focusHeight = world.getComponent<Component::Movement>(entityID).cameraFocusHeight;
                        }

                        // CÓDIGO MIGRADADO DO RAYCASTING
                        glm::vec3 rayOrigin = glm::vec3(transform.position.x,
                                                        100.0f, // Base alta
                                                        transform.position.z);
                        glm::vec3 rayDirection(0.0f, -1.0f, 0.0f);
                        float minDistance = std::numeric_limits<float>::max();
                        bool foundIntersection = false;

                        // Itera sobre as Meshes do Modelo (Modelo já obtido)
                        for (const auto &mesh_ptr : terrainModel->getMeshes())
                        {
                            const auto &vertices = mesh_ptr->getVertices();
                            const auto &indices = mesh_ptr->getIndices();

                            for (size_t i = 0; i < indices.size(); i += 3)
                            {
                                const glm::vec3 &v0 = vertices[indices[i]].Position;
                                const glm::vec3 &v1 = vertices[indices[i + 1]].Position;
                                const glm::vec3 &v2 = vertices[indices[i + 2]].Position;

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

                            // Aplicar a correção: A posição Y do personagem é exatamente a altura do chão.
                            transform.position.y = groundHeight;

                            // NOTA: A variável cameraFocusHeight ainda é usada no PlayerSystem
                            // para o m_camera.setTarget(), o que está correto.
                        }
                    }
                }
            }

        } // namespace System
    } // namespace ECS
} // namespace Engine