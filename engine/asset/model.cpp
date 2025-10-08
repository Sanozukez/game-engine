// engine/asset/model.cpp
#include "model.h"
#include "./../core/log.h"

#include "./../../engine/render/shader.h" // Incluir Shader para Mesh::draw

#include "../render/opengl_types.h"
#include <cstddef> // For offsetof
#include <format>

namespace Engine
{
    namespace Asset
    {

        // --- Mesh Class ---
        Mesh::Mesh(std::vector<Vertex> &&vertices, std::vector<uint32_t> &&indices, std::unique_ptr<Render::Material> material)
             : m_vertices(std::move(vertices)),
              m_indices(std::move(indices)),
              m_material(std::move(material))
        {
            setupMesh();
            Engine::Log::Info(std::format("Mesh: Created with {} vertices and {} indices.", m_vertices.size(), m_indices.size()));
        }

        Mesh::~Mesh()
        {
            if (m_VAO != 0)
            {
                glDeleteVertexArrays(1, &m_VAO);
                glDeleteBuffers(1, &m_VBO);
                glDeleteBuffers(1, &m_EBO);
            }
            Engine::Log::Trace("Mesh: Destructor called. OpenGL resources released.");
        }

        // NOVO: Construtor de Cópia (Necessário para Model::clone())
        Mesh::Mesh(const Mesh &other)
            : m_vertices(other.m_vertices),
              m_indices(other.m_indices),
              // Chama o clone do material (que clona as texturas)
              m_material(other.m_material ? other.m_material->clone() : nullptr)
        {
            // O construtor de cópia exige que os novos buffers OpenGL sejam criados.
            setupMesh();
            Engine::Log::Trace(std::format("Mesh: Copied (Deep Copy) and new OpenGL buffers created."));
        }

        void Mesh::setupMesh()
        {
            m_VAO = 0;
            m_VBO = 0;
            m_EBO = 0;

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
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, Position));
            // Normal (layout = 1)
            glEnableVertexAttribArray(1);
            glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, Normal));
            // Texture Coordinates (layout = 2)
            glEnableVertexAttribArray(2);
            glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, TexCoords));
            // Tangent (layout = 3)
            glEnableVertexAttribArray(3);
            glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, Tangent));

            glBindVertexArray(0); // Unbind VAO
            Engine::Log::Trace(std::format("Mesh: VAO ({}), VBO ({}), EBO ({}) configured.", m_VAO, m_VBO, m_EBO));
        }

        void Mesh::draw(const Render::Shader &shader) const
        {
            if (m_material)
            {
                m_material->activate(shader); // Ativa o material (configura uniforms)
            }

            glBindVertexArray(m_VAO);
            glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(m_indices.size()), GL_UNSIGNED_INT, 0);
            glBindVertexArray(0);

            if (m_material)
            {
                m_material->deactivate(); // Desativa o material
            }
        }

        // --- Model Class ---
        Model::Model() = default;
        Model::~Model() = default;

        void Model::addMesh(std::unique_ptr<Mesh> mesh)
        {
            if (mesh)
            {
                m_meshes.push_back(std::move(mesh));
                Engine::Log::Trace("Model: Mesh added.");
            }
            else
            {
                Engine::Log::Warn("Model: Attempting to add null mesh.");
            }
        }

        void Model::draw(const Render::Shader &shader) const
        {
            for (const auto &mesh : m_meshes)
            {
                if (mesh)
                {
                    mesh->draw(shader); // Passa o shader para Mesh::draw
                }
            }
        }

        // NOVO: Implementação da função clone() para Model
        std::unique_ptr<Model> Model::clone() const
        {
            // 1. Cria um novo Model vazio
            auto clonedModel = std::make_unique<Model>();

            // 2. Copia Profunda (Deep Copy) das Meshes
            // NOTA: Para que este loop funcione, a classe Mesh deve ter um método clone()!
            for (const auto &mesh : m_meshes)
            {
                if (mesh)
                {
                    // O mesh->clone() deve retornar um std::unique_ptr<Mesh> com novos VAOs/VBOs
                    // Se o seu Mesh tiver um construtor de cópia, podemos usá-lo:
                    clonedModel->m_meshes.push_back(std::make_unique<Mesh>(*mesh));
                    // ^^^^^ Assumindo construtor de cópia Mesh::Mesh(const Mesh&), que fará setupMesh().
                }
            }

            // 3. (Opcional) Copia outros dados do Model (se existirem)

            Engine::Log::Trace(std::format("Model: Clonado com sucesso! {} meshes copiadas.", clonedModel->m_meshes.size()));
            return clonedModel;
        }

    } // namespace Asset
} // namespace Engine