// // engine/game/player_spawner.cpp

#include "../../engine/render/opengl_types.h"
#include "player_spawner.h"
#include "player_character.h"
#include "game_object.h"

#include "../core/log.h"
#include "../core/config_manager.h"
#include "../asset/model.h"
#include "../asset/asset_manager.h"

#include "../../src/app/scene.h" 
#include "../../shared/mmap_format/SceneFileFormat.h" // Necessário para SceneNode

#include <glm/glm.hpp>
#include <format>
#include <stdexcept>

namespace Engine::Game {

void PlayerSpawner::Spawn(Engine::Scene& targetScene, 
                          Engine::Game::PlayerCharacter*& playerCharacter,
                          const SceneNode* spawnNode) 
{
    // 1. Acessa o Manager e resolve o ID
    Engine::Asset::AssetManager& assetManager = Engine::Asset::AssetManager::Get();

    try
    {
        // 1. ID FINAL: Obtém o ID a partir do nome lógico (sem hardcode numérico no C++)
        uint32_t characterAssetID = assetManager.getAssetIDByName("character_placeholder.glb"); 
        
        std::shared_ptr<Engine::Asset::Model> characterModel = assetManager.getModel(characterAssetID);

        if (!characterModel) {
            Engine::Log::Error("PlayerSpawner: Falha ao obter modelo do Player. Pulando spawn.");
            return;
        }

        // 2. Definição da Posição (CORREÇÃO FINAL DO Y-OFFSET)
        glm::vec3 spawnPosition(20.0f, 0.0f, 0.0f); // Fallback: (X=20.0, Y=0.0, Z=0.0)

        if (spawnNode) {
            // Usa X e Z do MMAP, e SOMA o offset de 0.9f ao Y do MMAP.
            // Isso garante que o personagem nasça ACIMA do chão da montanha.
            spawnPosition = glm::vec3(spawnNode->position[0], 
                                      spawnNode->position[1] + 0.9f, // <-- CORREÇÃO: Y do MMAP + Altura de Spawn
                                      spawnNode->position[2]);
        }
        
        // 3. Criação e Configuração
        std::unique_ptr<Engine::Asset::Model> clonedModel = characterModel->clone();
        auto characterObject = std::make_unique<Engine::Game::PlayerCharacter>(std::move(clonedModel));
        
       characterObject->setPosition(spawnPosition);

        Engine::Log::Info(std::format("Player Spawn: Node encontrado. Posição de destino: ({}, {}, {})", 
                              spawnPosition.x, spawnPosition.y, spawnPosition.z));

        // ... (Configurações JSON: CameraFocusHeight, MovementSpeed, etc.)
        auto &config = Engine::ConfigManager::Get();
        characterObject->setCameraFocusHeight(config.getValue<float>("character.player.camera_focus_height", 1.0f));
        characterObject->setMovementSpeed(config.getValue<float>("character.player.movement_speed", 5.0f));
        characterObject->setRotationSpeed(config.getValue<float>("character.player.rotation_speed_degrees", 75.0f));

        playerCharacter = characterObject.get(); // Atualiza o ponteiro na Scene
        targetScene.addGameObject(std::move(characterObject));
        Engine::Log::Info("GameObject Personagem carregado e configurado no Spawn Point.");
    }
    catch (const std::exception &e)
    {
        Engine::Log::Error(std::format("Erro ao carregar PlayerCharacter: {}", e.what()));
    }
}

} // namespace Engine::Game