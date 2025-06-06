// engine/asset/obj_loader.cpp
#include "obj_loader.h"
#include "./../core/log.h"         // Para logging
#include "./../core/path_utils.h"   // Para Engine::loadFileFromEngineAssets
#include <fstream>                  // Para leitura de arquivo
#include <sstream>                  // Para stringstream
#include <stdexcept>                // Para exceções
#include <unordered_map>            // Para otimização de vértices
#include "./../../engine/render/material.h" // Para material padrão (nullptr)

// Estruturas auxiliares para parsing (dentro do .cpp para não poluir o .h)
struct ObjVertex {
    unsigned int positionIndex;
    unsigned int texCoordIndex;
    unsigned int normalIndex;

    // Operador de comparação para usar como chave em unordered_map
    bool operator==(const ObjVertex& other) const {
        return positionIndex == other.positionIndex &&
               texCoordIndex == other.texCoordIndex &&
               normalIndex == other.normalIndex;
    }
};

// Functor de hash para ObjVertex
namespace std {
template <> struct hash<ObjVertex> {
    size_t operator()(const ObjVertex& v) const {
        // Combinação de hashes simples para fins de exemplo
        return hash<unsigned int>()(v.positionIndex) ^
               (hash<unsigned int>()(v.texCoordIndex) << 1) ^
               (hash<unsigned int>()(v.normalIndex) << 2);
    }
};
} // namespace std


namespace Engine {
namespace Asset {

std::unique_ptr<Model> ObjLoader::loadModel(const std::string& filePath) {
    Engine::Log::Info(std::format("ObjLoader: Tentando carregar modelo OBJ de '{}'", filePath));

    // Carrega o conteúdo do arquivo OBJ como uma string
    std::string objContent;
    try {
        objContent = Engine::loadFileFromEngineAssets(filePath);
    } catch (const std::exception& e) {
        Engine::Log::Error(std::format("ObjLoader: Falha ao ler arquivo OBJ: {}", e.what()));
        throw std::runtime_error(std::format("Falha ao carregar modelo OBJ: {}", e.what()));
    }

    std::vector<glm::vec3> tempPositions;
    std::vector<glm::vec2> tempTexCoords;
    std::vector<glm::vec3> tempNormals;
    std::vector<ObjVertex> tempIndices; // Índices lidos do arquivo OBJ (v/vt/vn)

    std::stringstream ss(objContent);
    std::string line;

    while (std::getline(ss, line)) {
        if (line.empty() || line[0] == '#') {
            continue; // Ignora linhas vazias ou comentários
        }

        std::istringstream iss(line);
        std::string prefix;
        iss >> prefix;

        if (prefix == "v") {
            glm::vec3 pos;
            iss >> pos.x >> pos.y >> pos.z;
            tempPositions.push_back(pos);
        } else if (prefix == "vt") {
            glm::vec2 tex;
            iss >> tex.x >> tex.y;
            // OBJ geralmente tem Y invertido para texturas, pode precisar ajustar:
            // tex.y = 1.0f - tex.y; 
            tempTexCoords.push_back(tex);
        } else if (prefix == "vn") {
            glm::vec3 normal;
            iss >> normal.x >> normal.y >> normal.z;
            tempNormals.push_back(normal);
        } else if (prefix == "f") {
            // Parsing de faces: v/vt/vn v/vt/vn v/vt/vn (ou variações)
            std::string vertexStr;
            while (iss >> vertexStr) {
                std::istringstream viss(vertexStr);
                std::string segment;
                unsigned int indices[3] = {0, 0, 0}; // pos, tex, normal

                // Divide a string (ex: "1/2/3") pelos '/'
                for (int i = 0; i < 3; ++i) {
                    if (std::getline(viss, segment, '/')) {
                        if (!segment.empty()) {
                            indices[i] = std::stoul(segment); // stoul para unsigned int
                        }
                    } else { // Se não houver mais segmentos, para (ex: "v" ou "v/vt")
                        break;
                    }
                }
                
                // OBJ usa índices base 1, então ajustamos para base 0
                tempIndices.push_back({indices[0] - 1, indices[1] - 1, indices[2] - 1});
            }
        }
    }

    // Agora, re-indexar os vértices para criar um único array de vértices e um de índices
    std::vector<Vertex> finalVertices;
    std::vector<unsigned int> finalIndices;
    std::unordered_map<ObjVertex, unsigned int> uniqueVertices; // Para evitar vértices duplicados

    for (const auto& objVert : tempIndices) {
        if (uniqueVertices.count(objVert)) {
            // Vértice já existe, adicione seu índice
            finalIndices.push_back(uniqueVertices[objVert]);
        } else {
            // Vértice é novo, adicione-o à lista final e registre seu índice
            Vertex newVertex;
            newVertex.Position = tempPositions[objVert.positionIndex]; 
            
            // Verificações de segurança para evitar acesso fora dos limites
            if (objVert.texCoordIndex < tempTexCoords.size()) {
                newVertex.TexCoords = tempTexCoords[objVert.texCoordIndex];
            } else {
                newVertex.TexCoords = glm::vec2(0.0f); // Padrão se não houver texCoords
                Engine::Log::Warn(std::format("ObjLoader: Vértice sem coordenadas de textura. Arquivo: {}", filePath));
            }

            if (objVert.normalIndex < tempNormals.size()) {
                newVertex.Normal = tempNormals[objVert.normalIndex];
            } else {
                newVertex.Normal = glm::vec3(0.0f, 1.0f, 0.0f); // Padrão se não houver normais
                Engine::Log::Warn(std::format("ObjLoader: Vértice sem normais. Arquivo: {}", filePath));
            }

            finalVertices.push_back(newVertex);
            unsigned int newIndex = static_cast<unsigned int>(finalVertices.size() - 1);
            finalIndices.push_back(newIndex);
            uniqueVertices[objVert] = newIndex;
        }
    }

    Engine::Log::Info(std::format("ObjLoader: Modelo '{}' carregado. Vértices: {}, Índices: {}",
                                  filePath, finalVertices.size(), finalIndices.size()));
    
    // Creates and returns the Model using the loaded data
    auto model = std::make_unique<Model>();
    // std::make_unique<Render::Material>() cria um material padrão (nullptr por enquanto)
    auto mesh = std::make_unique<Mesh>(std::move(finalVertices), std::move(finalIndices), std::make_unique<Render::Material>());
    model->addMesh(std::move(mesh)); // Adiciona a mesh ao modelo

    return model; // Retorna o modelo com a mesh
}

} // namespace Asset
} // namespace Engine