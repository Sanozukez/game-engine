// // engine/core/scene_node_fetcher.h

#pragma once

#include <vector>
#include <string>
#include <cstdint>

#include "../../shared/mmap_format/SceneFileFormat.h"
// Forward declarations para SceneNode
struct SceneNode; 

namespace Engine::Core {

class WorldDataFetcher {
public:
    /**
     * @brief Busca um node específico (Player Spawn, Terreno) pelo nome.
     * @param nodes O vetor de SceneNodes carregado do MMAP.
     * @param targetName O nome da string a ser buscada (Ex: "SPAWN_Player_Start").
     * @return O ponteiro para o SceneNode encontrado, ou nullptr.
     */
    static const SceneNode* FindNodeByName(const std::vector<SceneNode>& nodes, const char* targetName);

    // Busca o primeiro node com um tipo de entidade específico
    static const SceneNode* FindNodeByType(const std::vector<SceneNode>& nodes, EntityType type); // <-- NOVO
};

} // namespace Engine::Game