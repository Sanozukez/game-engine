// // engine/game/world_initializer.h
#pragma once

#include <cstdint> 

// Forward Declarations: Evitar includes pesados
namespace Engine {
    class Scene; // Para a referência na função Initialize
}

namespace Engine::Game {
    class PlayerCharacter; 
    class GameObject;
}

namespace Engine {
namespace Game {

/**
 * @brief Responsável por toda a lógica de inicialização da cena e do mundo.
 * * Este módulo segue o SRP: Ele orquestra o carregamento de arquivos de cena (.mmap)
 * e a subsequente instanciação de todos os GameObjects (Terreno, Static Meshes).
 * * Ele é chamado uma única vez por Engine::Scene::initialize().
 */
class WorldInitializer {
private:
    // Helper para isolar a criação do PlayerCharacter
    static void initializePlayerCharacter(Engine::Scene& targetScene, 
                                          Engine::Game::PlayerCharacter*& playerCharacter);

public:
    // Ponto de entrada estático para inicializar o mundo.
    static void Initialize(Engine::Scene& targetScene, 
                           Engine::Game::PlayerCharacter*& playerCharacter, 
                           Engine::Game::GameObject*& terrainObject);
};

} // namespace Game
} // namespace Engine