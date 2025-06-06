// engine/asset/obj_loader.h
#pragma once

#include "model.h" // Inclui a definição da classe Model
#include <string>    // Para o caminho do arquivo
#include <vector>    // Para std::vector (interno)
#include <glm/glm.hpp> // Para glm::vec3, glm::vec2 (interno)
#include <memory>    // **** NOVO: Para std::unique_ptr ****

namespace Engine {
namespace Asset {

// Classe ObjLoader: Responsável por carregar um arquivo .obj em um objeto Model
class ObjLoader {
public:
    // Método estático para carregar um modelo OBJ.
    // Retorna um unique_ptr para Engine::Asset::Model.
    // Lança exceção em caso de falha.
    static std::unique_ptr<Model> loadModel(const std::string& filePath);

private:
    // Construtor privado para evitar instanciação, pois é uma classe de utilidade estática
    ObjLoader() = delete;
};

} // namespace Asset
} // namespace Engine