// engine/asset/model.cpp
#include "model.h"
#include "./../core/log.h"

#include "./../../engine/render/shader.h" // Incluir Shader para Mesh::draw

#include <glad/gl.h>
#include <cstddef> // For offsetof
#include <format> 

namespace Engine {
namespace Asset {

// --- Mesh Class ---
Mesh::Mesh(std::vector<Vertex>&& vertices, std::vector<GLuint>&& indices, std::unique_ptr<Render::Material> material)
    : m_vertices(std::move(vertices)),
      m_indices(std::move(indices)),
      m_material(std::move(material)) {
    setupMesh();
    Engine::Log::Info(std::format("Mesh: Created with {} vertices and {} indices.", m_vertices.size(), m_indices.size()));
}

Mesh::~Mesh() {
    if (m_VAO != 0) { 
        glDeleteVertexArrays(1, &m_VAO);
        glDeleteBuffers(1, &m_VBO);
        glDeleteBuffers(1, &m_EBO); 
    }
    Engine::Log::Trace("Mesh: Destructor called. OpenGL resources released.");
}

void Mesh::setupMesh() {
    m_VAO = 0; m_VBO = 0; m_EBO = 0; 

    glGenVertexArrays(1, &m_VAO);
    glGenBuffers(1, &m_VBO);
    glGenBuffers(1, &m_EBO); 

    glBindVertexArray(m_VAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_VBO);

    glBufferData(GL_ARRAY_BUFFER, m_vertices.size() * sizeof(Vertex), m_vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_EBO); 
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_indices.size() * sizeof(GLuint), m_indices.data(), GL_STATIC_DRAW);

    // Vertex attributes configuration
    // Position (layout = 0)
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Position));
    // Normal (layout = 1)
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Normal));
    // Texture Coordinates (layout = 2)
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, TexCoords));
    // Tangent (layout = 3)
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Tangent));


    glBindVertexArray(0); // Unbind VAO
    Engine::Log::Trace(std::format("Mesh: VAO ({}), VBO ({}), EBO ({}) configured.", m_VAO, m_VBO, m_EBO));
}

void Mesh::draw(const Render::Shader& shader) const { 
    if (m_material) {
        m_material->activate(shader); // Ativa o material (configura uniforms)
    }

    glBindVertexArray(m_VAO);
    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(m_indices.size()), GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);

    if (m_material) {
        m_material->deactivate(); // Desativa o material
    }
}

// --- Model Class ---
Model::Model() = default;
Model::~Model() = default;

void Model::addMesh(std::unique_ptr<Mesh> mesh) {
    if (mesh) {
        m_meshes.push_back(std::move(mesh));
        Engine::Log::Trace("Model: Mesh added.");
    } else {
        Engine::Log::Warn("Model: Attempting to add null mesh.");
    }
}

void Model::draw(const Render::Shader& shader) const { 
    for (const auto& mesh : m_meshes) {
        if (mesh) {
            mesh->draw(shader); // Passa o shader para Mesh::draw
        }
    }
}

} // namespace Asset
} // namespace Engine