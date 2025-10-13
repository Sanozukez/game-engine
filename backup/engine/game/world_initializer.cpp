// // engine/game/world_initializer.cpp

#include "world_initializer.h"
#include "player_spawner.h"
#include "world_data_fetcher.h"
#include "../core/log.h"
#include "../asset/gltf_loader.h"

#include "../asset/asset_manager.h"

#include "../../src/app/scene.h" // Necessário para que targetScene possa chamar addGameObject()

// Includes do pipeline MMAP
#include "../../shared/mmap_format/SceneFileFormat.h"
#include "../core/SceneLoader.h"

#include "game_object.h"
#include "player_character.h"
#include "../asset/model.h"
#include "../core/config_manager.h"

#include <glm/gtc/quaternion.hpp>
#include <glm/glm.hpp>
#include <format>
#include <stdexcept>

namespace Engine::Game
{
    void WorldInitializer::Initialize(Engine::Scene &targetScene,
                                      Engine::Game::PlayerCharacter *&playerCharacter,
                                      Engine::Game::GameObject *&terrainObject)
{
    // 1. CARREGAMENTO DO MAPA (Configuração Dinâmica)
    auto &config = Engine::ConfigManager::Get();
    std::string mapPath = config.getValue<std::string>("world.start_map_path", "assets/models/default.mmap"); 

    Engine::SceneLoader sceneLoader;
    bool loadSuccess = sceneLoader.loadMapBinary(mapPath);

    if (!loadSuccess) {
        Engine::Log::Error("Falha crítica ao carregar arquivo de cena (.mmap).");
        return;
    }

    const auto &loaded_nodes = sceneLoader.nodes;
    Engine::Asset::AssetManager &assetManager = Engine::Asset::AssetManager::Get();

    // VARIÁVEIS DE CONTROLE
    const SceneNode *playerSpawnNode = nullptr;
    const SceneNode *terrainNode = nullptr;


    // 2. BUSCA DE NODES DE CONTROLE (Utiliza o Data Fetcher e resolve a posição do Player)

    // A. Busca dos Nodes Principais
    terrainNode     = WorldDataFetcher::FindNodeByType(loaded_nodes, EntityType::TYPE_TERRAIN_BASE);
    playerSpawnNode = WorldDataFetcher::FindNodeByName(loaded_nodes, "SPAWN_Player_Start");
    
    // Fallback de Busca para o Terreno
    if (!terrainNode) {
        terrainNode = WorldDataFetcher::FindNodeByName(loaded_nodes, "TER_Base_Mesh");
    }

    // 3. CRIAÇÃO DOS OBJETOS DE CONTROLE (SETUP)

    // A. Player Spawn (CORRIGIDO: PlayerSpawner usa o node para posição)
    PlayerSpawner::Spawn(targetScene, playerCharacter, playerSpawnNode);


    // B. Lógica do Terreno (Instanciação do Objeto Único)
    if (terrainNode)
    {
        uint32_t terrainAssetID = terrainNode->asset_reference_id; 
        std::shared_ptr<Engine::Asset::Model> terrainModel = assetManager.getModel(terrainAssetID);

        if (terrainModel)
        {
            auto terrainObjectPtr = std::make_unique<Engine::Game::GameObject>(terrainModel->clone());
            
            // Atribuição de nome e posição
            terrainObjectPtr->name = "Terrain_MMAP";
            terrainObjectPtr->setPosition(glm::vec3(terrainNode->position[0], terrainNode->position[1], terrainNode->position[2]));
            
            terrainObject = terrainObjectPtr.get();
            targetScene.addGameObject(std::move(terrainObjectPtr));
            Engine::Log::Info("GameObject Terreno BASE carregado com sucesso!");
        }
        else
        {
            Engine::Log::Error(std::format("Falha crítica ao obter Terreno (ID {}). Verifique o dicionário.", terrainAssetID));
        }
    }
    else
    {
        Engine::Log::Error("AVISO: Nenhum Node TER_BASE encontrado para ser o Terreno.");
    }


    // 4. INSTANCIAÇÃO DE OBJETOS REPETITIVOS (O Loop Final)
    for (const auto &node : loaded_nodes)
    {
        // Ignora os nodes de controle (já criados ou lógicos)
        if (node.type == EntityType::TYPE_TERRAIN_BASE || 
            node.type == EntityType::TYPE_ARRAY_START ||
            node.asset_reference_id == 0) // Ignora marcadores lógicos sem asset
        {
            continue;
        }

        // Se for um objeto que precisa ser instanciado e adicionado à cena:
        if (node.type == EntityType::TYPE_STATIC_MESH)
        {
            uint32_t asset_id = node.asset_reference_id;
            std::shared_ptr<Engine::Asset::Model> baseModel = assetManager.getModel(asset_id);

            if (!baseModel)
            {
                std::string asset_name_for_log = assetManager.getAssetPathByID(asset_id);
                Engine::Log::Error(std::format("Falha ao obter modelo '{}' (ID: {}). Pulando instanciação.",
                                               asset_name_for_log, asset_id));
                continue;
            }

            // Criação e configuração (Muros, NPCs, Props)
            auto moduleObject = std::make_unique<Engine::Game::GameObject>(baseModel->clone());

            // Conversão do Quatérnio e Posição
            glm::quat rotationQuat(node.rotation_quat[0], node.rotation_quat[1], node.rotation_quat[2], node.rotation_quat[3]);

            moduleObject->setPosition(glm::vec3(node.position[0], node.position[1], node.position[2]));
            moduleObject->setRotation(rotationQuat);

            targetScene.addGameObject(std::move(moduleObject));
        }
    }
    
    Engine::Log::Info("World Initializer: Carregamento de mapa concluído.");
}

} // namespace Engine::Game