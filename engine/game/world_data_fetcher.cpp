// // engine/game/world_data_fetcher.cpp

#include "world_data_fetcher.h"
#include <cstring>
#include "../../shared/mmap_format/SceneFileFormat.h" // Para a struct SceneNode

namespace Engine::Game {

const SceneNode* WorldDataFetcher::FindNodeByName(const std::vector<SceneNode>& nodes, const char* targetName) {
    if (!targetName) return nullptr;
    size_t targetLen = std::strlen(targetName); 

    for (const auto& node : nodes) {
        
        // 1. Otimização: Se o comprimento é diferente, pule (a menos que o nome seja menor que 64)
        // Usamos uma string temporária para garantir a null-termination (a mais segura)
        std::string nodeNameStr(node.name); 

        // 2. CRÍTICO: Comparar a string std::string (que é mais segura)
        if (nodeNameStr == targetName) {
            return &node;
        }
        
        // 3. Fallback: Se a comparação std::string falhar, usamos strncmp
        if (std::strncmp(node.name, targetName, targetLen) == 0 && node.name[targetLen] == '\0') {
            // Verifica se os primeiros N caracteres são iguais E se o N+1 caractere é nulo (terminação)
            return &node;
        }
    }
    return nullptr;
}

// Implementação da busca por tipo de entidade
const SceneNode* WorldDataFetcher::FindNodeByType(const std::vector<SceneNode>& nodes, EntityType type) {
    for (const auto& node : nodes) {
        if (node.type == type) {
            return &node;
        }
    }
    return nullptr;
}

} // namespace Engine::Game