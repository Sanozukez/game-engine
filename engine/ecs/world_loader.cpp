// // engine/ecs/world_loader.cpp

#include "world_loader.h"
#include "../core/log.h"
#include "../asset/asset_manager.h"

// Includes do pipeline MMAP
#include "../../shared/mmap_format/SceneFileFormat.h"
#include "../core/SceneLoader.h"
#include "../core/config_manager.h"
#include "../math/vec3.h"
#include "../../math/quat.h" // Necessário para a rotação

// Includes dos Componentes e ECS World
#include "world.h" 
#include "components/terrain_component.h"
#include "components/player_component.h"
#include "components/movement_component.h"
#include "components/mesh_component.h" 

// Includes dos Utilitários (O que era 'game/...')
#include "../core/scene_node_fetcher.h" // <--- CAMINHO CORRETO ASSUMIDO APÓS SUA CORREÇÃO

#include <glm/gtc/quaternion.hpp>
#include <glm/glm.hpp>
#include <format>
#include <stdexcept> // Para std::runtime_error

namespace Engine {
namespace ECS {

using namespace Engine::ECS::Component;
using namespace Engine::Asset;
// NOTA: WorldDataFetcher agora é acessado via Engine::Core

// ------------------------------------------------------------------------
// NOVO MÉTODO DE CARREGAMENTO ECS
// ------------------------------------------------------------------------

bool WorldLoader::Load(World& world, EntityID playerID)
{
    // 1. CARREGAMENTO DO MAPA (Configuração Dinâmica)
    auto &config = Engine::ConfigManager::Get();
    std::string mapPath = config.getValue<std::string>("world.start_map_path", "assets/models/default.mmap"); 

    Engine::SceneLoader sceneLoader;
    if (!sceneLoader.loadMapBinary(mapPath)) {
        Engine::Log::Critical("Falha crítica ao carregar arquivo de cena (.mmap).");
        return false;
    }

    const auto &loaded_nodes = sceneLoader.nodes;
    AssetManager &assetManager = AssetManager::Get();

    // VARIÁVEIS DE CONTROLE E BUSCA DE NODES
    // NOTA: O WorldDataFetcher é chamado via Engine::Core::
    const SceneNode *playerSpawnNode = Engine::Core::WorldDataFetcher::FindNodeByName(loaded_nodes, "SPAWN_Player_Start");
    const SceneNode *terrainNode = Engine::Core::WorldDataFetcher::FindNodeByType(loaded_nodes, EntityType::TYPE_TERRAIN_BASE);
    if (!terrainNode) {
        terrainNode = Engine::Core::WorldDataFetcher::FindNodeByName(loaded_nodes, "TER_Base_Mesh");
    }


    // 2. CRIAÇÃO E CONFIGURAÇÃO DO PLAYER (Entity que já existe no App::run)
    
    // const EntityID playerID = world.getSingleEntityWith<Player>();
    
    if (playerID != INVALID_ENTITY_ID) {
        Component::Transform& transform = world.getTransform(playerID);
        Component::Movement& movement = world.getComponent<Movement>(playerID); 
        
        // --- LÓGICA DE SPAWN MIGRADA (Incluindo Offset) ---
        glm::vec3 spawnPosition(0.0f, 0.0f, 0.0f); 
        float spawnYOffset = 0.0f; 

        if (playerSpawnNode) 
        {
            // Aplica a posição de spawn + o Y offset (necessário para não cair no chão)
            spawnPosition = glm::vec3(
                playerSpawnNode->position[0], 
                playerSpawnNode->position[1] + spawnYOffset, 
                playerSpawnNode->position[2]
            );
        }
        
        // Aplicação da Posição
        transform.position = Engine::Math::Vec3(spawnPosition);

        // Configuração de Lógica e Velocidade (do JSON)
        movement.cameraFocusHeight = config.getValue<float>("character.player.camera_focus_height", 1.0f);
        movement.movementSpeed = config.getValue<float>("character.player.movement_speed", 5.0f);
        movement.rotationSpeed = config.getValue<float>("character.player.rotation_speed_degrees", 75.0f);

        Engine::Log::Info(std::format("Player Entity (ID {}) spawnada em ({}, {}, {}) com offset.", playerID, spawnPosition.x, spawnPosition.y, spawnPosition.z));
    }


    // 3. Lógica do Terreno (Instanciação como Entity com Tag Terrain)
    if (terrainNode)
    {
        uint32_t terrainAssetID = terrainNode->asset_reference_id; 
        
        EntityID terrainEntity = world.createEntity();
        
        world.addMesh(terrainEntity, terrainAssetID);
        world.addComponent<Terrain>(terrainEntity); 

        // Configuração da Posição
        Component::Transform& transform = world.getTransform(terrainEntity);
        transform.position = Engine::Math::Vec3(terrainNode->position[0], terrainNode->position[1], terrainNode->position[2]);
        
        Engine::Log::Info("Terrain Entity BASE carregada como componente ECS!");
    }


    // 4. INSTANCIAÇÃO DE OBJETOS REPETITIVOS (Muros, Props)
    for (const auto &node : loaded_nodes)
    {
        // Ignora os nodes de controle
        if (node.type == EntityType::TYPE_TERRAIN_BASE || 
            node.type == EntityType::TYPE_ARRAY_START || 
            node.asset_reference_id == 0 ||
            (playerSpawnNode && &node == playerSpawnNode) // Ignora o node de spawn do player
            ) continue; 

        if (node.type == EntityType::TYPE_STATIC_MESH)
        {
            uint32_t asset_id = node.asset_reference_id;
            
            if (!assetManager.isAssetAvailable(asset_id)) {
                 Engine::Log::Error(std::format("Falha ao obter modelo (ID: {}). Pulando instanciação.", asset_id));
                 continue;
            }

            // Criação e configuração da Entity
            EntityID moduleEntity = world.createEntity();
            world.addMesh(moduleEntity, asset_id);

            // Rotação Quatérnion
            glm::quat rotationQuat(node.rotation_quat[0], node.rotation_quat[1], node.rotation_quat[2], node.rotation_quat[3]);

            // MODIFICAÇÃO DIRETA DOS COMPONENTES (Substituindo setRotation/setPosition)
            Component::Transform& transform = world.getTransform(moduleEntity);
            transform.position = Engine::Math::Vec3(node.position[0], node.position[1], node.position[2]);
            transform.rotation = Engine::Math::Quat(rotationQuat);
        }
    }
    
    Engine::Log::Info("World Loader: Carregamento de mapa concluído. Entities criadas.");
    return true;
}

} // namespace ECS
} // namespace Engine
