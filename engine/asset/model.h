// engine/asset/model.h
#pragma once

#include <vector>
#include <string>
#include <memory> // Para std::unique_ptr

#include <glm/glm.hpp>

#include "./../../engine/render/material.h"

// Forward declaration para Shader (ainda necessário para Mesh::draw e Model::draw)
namespace Engine
{
    namespace Render
    {
        class Shader;
    }
} // namespace Engine

namespace Engine
{
    namespace Asset
    {

        // Estrutura para representar um único vértice, incluindo Posição, Normal e Coordenadas de Textura
        struct Vertex
        {
            glm::vec3 Position;
            glm::vec3 Normal;
            glm::vec2 TexCoords;
            glm::vec3 Tangent;
            // glm::vec3 Bitangent;
        };

        // Classe para representar uma única malha (Mesh)
        class Mesh
        {
        public:
            // Construtor: usa rvalue references (&&) para mover dados eficientemente
            Mesh(std::vector<Vertex> &&vertices, std::vector<uint32_t> &&indices, std::unique_ptr<Render::Material> material);
            ~Mesh();

            // Construtor de Cópia para Clone de mesh
            Mesh(const Mesh &other);
            
            void draw(Engine::Render::Shader &shader); 

            size_t getVertexCount() const { return m_vertices.size(); }
            size_t getIndexCount() const { return m_indices.size(); }

            // **** NOVO: Getter para o material da mesh ****
            const Render::Material *getMaterial() const { return m_material.get(); }

            // --- NOVOS MÉTODOS GETTER ADICIONADOS ---
            // Retornam uma referência constante aos vetores de dados, permitindo a leitura
            // de forma eficiente (sem copiar os dados) e segura (sem permitir modificação).
            const std::vector<Vertex> &getVertices() const { return m_vertices; }
            const std::vector<uint32_t> &getIndices() const { return m_indices; }

        private:
            // Dados do Mesh
            std::vector<Vertex> m_vertices;

            // Assumimos que m_indices usa GLuint, mas para isolar o GLAD,
            // usaremos uint32_t (que é o tamanho do GLuint, mas sem o include).
            // NOTA: Se você já usa GLuint aqui, podemos ter que mudar para uint32_t para ser limpo.
            std::vector<uint32_t> m_indices; // <-- SUBSTITUÍDO GLuint por uint32_t (Limpeza)

            std::unique_ptr<Render::Material> m_material; // PBR material of the mesh

            // IDs de Buffer/Array (SUBSTITUÍDO GLuint por unsigned int)
            unsigned int m_VAO;
            unsigned int m_VBO;
            unsigned int m_EBO;

            void setupMesh();
        };

        // Classe para representar um Modelo (que pode conter múltiplas meshes)
        class Model
        {
        public:
            Model();
            ~Model();

            void addMesh(std::unique_ptr<Mesh> mesh);

            // CORREÇÃO CRÍTICA: Precisa aceitar o Shader E a matriz para o ECS
            // void draw(const Render::Shader &shader, const glm::mat4 &viewProjectionModel) const;
            void draw(Engine::Render::Shader &shader); 

            const std::vector<std::unique_ptr<Mesh>> &getMeshes() const { return m_meshes; }

            // NOVO: Declaração da função clone para permitir a instanciação
            // Retorna uma cópia profunda (deep copy) do Model como um ponteiro único.
            std::unique_ptr<Model> clone() const;

        private:
            std::vector<std::unique_ptr<Mesh>> m_meshes;
        };

    } // namespace Asset
} // namespace Engine