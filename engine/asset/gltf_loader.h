// engine/asset/gltf_loader.h
#pragma once

#include <string>
#include <memory> // Para std::unique_ptr
#include <vector>

// Forward declarations para as classes que o GLTFLoader vai interagir/criar
namespace Engine {
    namespace Asset {
        class Model; // A classe Model que você já tem
    }
    namespace Render {
        class Material; // A nova classe Material que vamos definir
        class Texture;  // A classe Texture que você já tem
    }
} // namespace Engine

namespace Engine {
namespace Asset {

// A classe GLTFLoader será responsável por carregar um arquivo glTF
// e converter seus dados em um Engine::Asset::Model
class GLTFLoader {
public:
    // Carrega um modelo glTF do caminho especificado.
    // Retorna um unique_ptr para o modelo carregado, ou nullptr/lança exceção em caso de falha.
    static std::unique_ptr<Model> loadGLTF(const std::string& filePath);

private:
    // Métodos auxiliares privados para parsing e conversão (serão implementados no .cpp)
};

} // namespace Asset
} // namespace Engine