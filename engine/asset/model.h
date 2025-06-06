// engine/asset/model.h
#pragma once

#include <vector>
#include <string>
#include <memory> // Para std::unique_ptr
#include <glad/gl.h> // Para GLuint

#include <glm/glm.hpp>

#include "./../../engine/render/material.h" 

// Forward declaration para Shader (ainda necessário para Mesh::draw e Model::draw)
namespace Engine {
namespace Render {
    class Shader; 
}
} // namespace Engine

namespace Engine {
namespace Asset {

// Estrutura para representar um único vértice, incluindo Posição, Normal e Coordenadas de Textura
struct Vertex {
    glm::vec3 Position;
    glm::vec3 Normal;
    glm::vec2 TexCoords;
    glm::vec3 Tangent; 
    // glm::vec3 Bitangent; 
};

// Classe para representar uma única malha (Mesh)
class Mesh {
public:
    // Construtor: usa rvalue references (&&) para mover dados eficientemente
    Mesh(std::vector<Vertex>&& vertices, std::vector<GLuint>&& indices, std::unique_ptr<Render::Material> material);
    ~Mesh();

    void draw(const Render::Shader& shader) const; 

    size_t getVertexCount() const { return m_vertices.size(); }
    size_t getIndexCount() const { return m_indices.size(); }

    // **** NOVO: Getter para o material da mesh ****
    const Render::Material* getMaterial() const { return m_material.get(); }


private:
    std::vector<Vertex> m_vertices;
    std::vector<GLuint> m_indices;
    std::unique_ptr<Render::Material> m_material; // PBR material of the mesh

    GLuint m_VAO, m_VBO, m_EBO; 

    void setupMesh(); 
};

// Classe para representar um Modelo (que pode conter múltiplas meshes)
class Model {
public:
    Model(); 
    ~Model();

    void addMesh(std::unique_ptr<Mesh> mesh); 
    void draw(const Render::Shader& shader) const; // Desenha todas as meshes do modelo

    const std::vector<std::unique_ptr<Mesh>>& getMeshes() const { return m_meshes; }

private:
    std::vector<std::unique_ptr<Mesh>> m_meshes; 
};

} // namespace Asset
} // namespace Engine