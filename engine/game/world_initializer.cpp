// // engine/game/world_initializer.cpp

#include "world_initializer.h"
#include "../core/log.h"
#include "../asset/asset_manager.h"
#include "../asset/gltf_loader.h"

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

    // =========================================================================
    // HELPER: Inicialização do Personagem (Movido de Scene::initialize)
    // A única responsabilidade é criar o PlayerCharacter
    // =========================================================================

    void WorldInitializer::initializePlayerCharacter(Engine::Scene &targetScene,
                                                     Engine::Game::PlayerCharacter *&playerCharacter)
    {
        try
        {
            // ID Hash literal de "character_placeholder.glb"
            uint32_t characterAssetID = 614879287;

            // Acessa o Manager e usa o ID correto
            std::shared_ptr<Engine::Asset::Model> characterModel = Engine::Asset::AssetManager::Get().getModel(characterAssetID);

            if (characterModel)
            {
                // 1. Cria um unique_ptr<Model> para ser movido
                std::unique_ptr<Engine::Asset::Model> clonedModel = characterModel->clone();

                // 2. Cria o PlayerCharacter, movendo o Model clonado
                auto characterObject = std::make_unique<Engine::Game::PlayerCharacter>(std::move(clonedModel));

                // Posição inicial hardcoded (deve vir de um SPAWN_Player_Start do MMAP no futuro)
                characterObject->setPosition(glm::vec3(0.0f, 0.9f, 0.0f));

                auto &config = Engine::ConfigManager::Get();
                characterObject->setCameraFocusHeight(config.getValue<float>("character.player.camera_focus_height", 1.0f));
                characterObject->setMovementSpeed(config.getValue<float>("character.player.movement_speed", 5.0f));
                characterObject->setRotationSpeed(config.getValue<float>("character.player.rotation_speed_degrees", 75.0f));

                playerCharacter = characterObject.get(); // Atualiza o ponteiro na Scene
                targetScene.addGameObject(std::move(characterObject));
                Engine::Log::Info("GameObject Personagem carregado e configurado!");
            }
        }
        catch (const std::exception &e)
        {
            Engine::Log::Error(std::format("Erro ao carregar PlayerCharacter: {}", e.what()));
        }
    }

    // =========================================================================
    // FUNÇÃO PRINCIPAL: Carrega o Mundo MMAP (Lógica Principal)
    // =========================================================================

    void WorldInitializer::Initialize(Engine::Scene &targetScene,
                                      Engine::Game::PlayerCharacter *&playerCharacter,
                                      Engine::Game::GameObject *&terrainObject)
    {

        // 1. Instanciar o SceneLoader e carregar o MMAP
        // Obtém o caminho do mapa do JSON de configuração
        auto &config = Engine::ConfigManager::Get();
        std::string mapPath = config.getValue<std::string>("world.start_map_path", "assets/models/default.mmap"); // Com fallback

        Engine::SceneLoader sceneLoader;
        bool loadSuccess = sceneLoader.loadMapBinary(mapPath); // Usa a variável!

        if (!loadSuccess)
        {
            Engine::Log::Error("Falha crítica ao carregar arquivo de cena (.mmap).");
            return;
        }

        const auto &loaded_nodes = sceneLoader.nodes;
        Engine::Asset::AssetManager &assetManager = Engine::Asset::AssetManager::Get();

        // 2. LÓGICA DO TERRENO PRINCIPAL (Busca e Instanciação)
        const SceneNode *terrainNode = nullptr;
        for (const auto &node : loaded_nodes)
        {
            if (node.type == EntityType::TYPE_TERRAIN_BASE)
            {
                terrainNode = &node;
                break;
            }
        }

        if (terrainNode)
        {
            // ID Hash literal do arquivo GLB que você quer carregar
            uint32_t terrainAssetID = 2727254143;

            std::shared_ptr<Engine::Asset::Model> terrainModel = assetManager.getModel(terrainAssetID);

            if (terrainModel)
            {
                auto terrainObjectPtr = std::make_unique<Engine::Game::GameObject>(terrainModel->clone());
                terrainObjectPtr->name = "Terrain_MMAP";
                terrainObjectPtr->setPosition(glm::vec3(terrainNode->position[0], terrainNode->position[1], terrainNode->position[2]));

                terrainObject = terrainObjectPtr.get();
                targetScene.addGameObject(std::move(terrainObjectPtr));
                Engine::Log::Info("GameObject Terreno BASE carregado via MMAP!");
            }
        }
        else
        {
            Engine::Log::Error("AVISO: Nenhum Node TER_BASE encontrado para ser o Terreno.");
        }

        // 3. ITERAÇÃO PARA OUTROS STATIC MESHES / ARRAYS
        for (const auto &node : loaded_nodes)
        {
            if (node.type == EntityType::TYPE_TERRAIN_BASE || node.type == EntityType::TYPE_ARRAY_START)
            {
                continue;
            }

            if (node.type == EntityType::TYPE_STATIC_MESH)
            {

                uint32_t asset_id = node.asset_reference_id;
                std::shared_ptr<Engine::Asset::Model> baseModel = assetManager.getModel(asset_id);

                if (!baseModel)
                {
                    // Traduz ID para nome apenas para o log de erro!
                    std::string asset_name_for_log = assetManager.getAssetPathByID(asset_id);
                    Engine::Log::Error(std::format("Falha ao obter modelo '{}' (ID: {}). Pulando instanciação.",
                                                   asset_name_for_log, asset_id));
                    continue;
                }

                // Instanciação: Aplica Posição e Rotação calculada pelo Compiler
                auto moduleModelCopy = baseModel->clone();
                auto moduleObject = std::make_unique<Engine::Game::GameObject>(std::move(moduleModelCopy));

                // Conversão do Quatérnio (Lógica que resolveu o problema de rotação de 45 graus)
                glm::quat rotationQuat(node.rotation_quat[0], node.rotation_quat[1], node.rotation_quat[2], node.rotation_quat[3]);

                moduleObject->setPosition(glm::vec3(node.position[0], node.position[1], node.position[2]));
                moduleObject->setRotation(rotationQuat);

                targetScene.addGameObject(std::move(moduleObject));
            }
        }

        // 4. Inicializa PlayerCharacter (usa o helper)
        initializePlayerCharacter(targetScene, playerCharacter);

        Engine::Log::Info("World Initializer: Carregamento de mapa concluído.");
    }

} // namespace Engine::Game