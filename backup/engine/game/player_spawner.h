// // engine/game/player_spawner.h

#pragma once

#include <memory>
#include <string>
#include <cstdint> // Para uint32_t

// Forward Declarations
namespace Engine {
    class Scene; 
    namespace Game {
        class PlayerCharacter;
    }
}

// Incluir o SceneNode para a posição
struct SceneNode; 

namespace Engine::Game {

// PlayerSpawner: Única responsabilidade é criar e configurar o PlayerCharacter
class PlayerSpawner {
public:
    /**
     * @brief Carrega o PlayerCharacter, aplica configurações JSON e define o spawn.
     * @param targetScene O container para adicionar o objeto.
     * @param playerCharacter Ponteiro a ser atualizado (o Player em si).
     * @param spawnNode O SceneNode com a posição de spawn (ou nullptr para fallback).
     */
    static void Spawn(Engine::Scene& targetScene, 
                      Engine::Game::PlayerCharacter*& playerCharacter,
                      const SceneNode* spawnNode);
};

} // namespace Engine::Game