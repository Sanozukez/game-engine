// engine/asset/model.cpp
#include "model.h"
#include "./../core/log.h"
#include "./../render/shader.h" // Incluir Shader para Mesh::draw
#include "../render/opengl_types.h"
#include "./../ecs/components/animation_component.h"
#include "../ecs/components/component_signature.h"
#include <cstddef> // For offsetof
#include <format>
#include <vector>
#include <glm/gtc/matrix_inverse.hpp> // <-- ADICIONAR (para glm::inverse)
#include "skeleton.h"                 // <-- ADICIONAR
#include "animation.h"

using namespace Engine::ECS::Component;

namespace Engine
{
    namespace Asset
    {
        // --- Mesh Class ---
        Mesh::Mesh(std::vector<Vertex> &&vertices,
                   std::vector<uint32_t> &&indices,
                   std::unique_ptr<Render::Material> material,
                   const glm::mat4 &nodeTransform)
            : m_vertices(std::move(vertices)), m_indices(std::move(indices)), m_material(std::move(material)), m_nodeTransform(nodeTransform)
        {
            setupMesh();
        }

        // Construtor “antigo” (compat) → node = Identity
        Mesh::Mesh(std::vector<Vertex> &&vertices,
                   std::vector<uint32_t> &&indices,
                   std::unique_ptr<Render::Material> material)
            : m_vertices(std::move(vertices)), m_indices(std::move(indices)), m_material(std::move(material)), m_nodeTransform(1.0f)
        {
            setupMesh();
        }

        Mesh::Mesh(const Mesh &other)
            : m_vertices(other.m_vertices), m_indices(other.m_indices), m_material(other.m_material ? other.m_material->clone() : nullptr), m_nodeTransform(other.m_nodeTransform)
        {
            setupMesh();
        }

        Mesh::~Mesh()
        {
            if (m_VAO != 0)
            {
                glDeleteVertexArrays(1, &m_VAO);
                glDeleteBuffers(1, &m_VBO);
                glDeleteBuffers(1, &m_EBO);
            }
            Engine::Core::Log::Trace("Mesh: Destructor called. OpenGL resources released.");
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
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_indices.size() * sizeof(uint32_t), m_indices.data(), GL_STATIC_DRAW);

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
            // Tangent (location = 3)
            glEnableVertexAttribArray(3);
            glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, Tangent));

            // BoneIDs (location = 4)  **inteiro**
            glEnableVertexAttribArray(4);
            glVertexAttribIPointer(4, 4, GL_INT, sizeof(Vertex), (void *)offsetof(Vertex, BoneIDs));

            // Weights (location = 5)
            glEnableVertexAttribArray(5);
            glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, Weights));

            glBindVertexArray(0);
        }

        void Engine::Asset::Mesh::draw(Engine::Render::Shader &shader)
        {
            shader.setMat4("uNode", m_nodeTransform);
            // 1. Ativar o Shader (passado como argumento)
            // O Material::activate precisa do Shader para configurar uniforms.
            if (m_material)
            {
                // NOTA: O Material deve ser responsável por setar os uniforms específicos dele.
                m_material->activate(shader);
            }

            // 3. Desenhar
            glBindVertexArray(m_VAO);
            glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(m_indices.size()), GL_UNSIGNED_INT, 0);
            glBindVertexArray(0);

            // 4. Desativar
            if (m_material)
            {
                m_material->deactivate();
            }
        }

        // -------------------------------------------------------------
        // Model Class
        // -------------------------------------------------------------
        Model::Model() = default;
        Model::~Model() = default;

        void Model::addMesh(std::unique_ptr<Mesh> mesh)
        {
            if (mesh)
            {
                m_meshes.push_back(std::move(mesh));
                Engine::Core::Log::Trace("Model: Mesh added.");
            }
            else
            {
                Engine::Core::Log::Warn("Model: Attempting to add null mesh.");
            }
        }

        // -------------------------------------------------------------
        // MÉTODOS DE SKELETON
        // -------------------------------------------------------------

        void Model::addNode(const Node &node)
        {
            m_nodeHierarchy[node.name] = node;
        }

        // NOVO: Shell de getNodeChildren
        const std::vector<std::string> Model::getNodeChildren(const std::string &nodeName) const
        {
            if (m_nodeHierarchy.count(nodeName))
            {
                return m_nodeHierarchy.at(nodeName).childrenNames;
            }
            return {};
        }

        // NOVO: Shell de getNodeLocalTransform
        const glm::mat4 Model::getNodeLocalTransform(const std::string &nodeName) const
        {
            if (m_nodeHierarchy.count(nodeName))
            {
                return m_nodeHierarchy.at(nodeName).localTransform;
            }
            // Retorna Identity como fallback seguro (como o shell)
            return glm::mat4(1.0f);
        }

        void Model::addAnimation(uint32_t nameHash, std::unique_ptr<Animation> anim)
        {
            m_animations[nameHash] = std::move(anim);
        }

        const Animation *Model::getAnimation(uint32_t nameHash) const
        {
            auto it = m_animations.find(nameHash); // <-- CORRIGIDO
            return (it != m_animations.end()) ? it->second.get() : nullptr;
        }

        void Model::draw(Engine::Render::Shader &shader, const std::vector<glm::mat4> *boneTransforms)
        {
            // NOVO: Se boneTransforms estiver presente, enviar para o shader.
            if (boneTransforms != nullptr)
            {
                // Certifique-se de que o MAX_BONES está visível. Usamos a constante do Componente.
                for (size_t i = 0; i < boneTransforms->size() && i < Skeleton::MAX_BONES; ++i)
                {
                    std::string uniformName = "uBoneTransforms[" + std::to_string(i) + "]";
                    shader.setMat4(uniformName, boneTransforms->at(i));
                }
                // Nota: A flag uIsAnimated já foi setada no Renderer::submit.
            }
            // A implementação correta é:
            for (const auto &mesh_ptr : m_meshes)
            {
                // mesh_ptr é um std::unique_ptr<Mesh>, então usamos ->
                mesh_ptr->draw(shader);
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

            Engine::Core::Log::Trace(std::format("Model: Clonado com sucesso! {} meshes copiadas.", clonedModel->m_meshes.size()));
            return clonedModel;
        }

        // Implementação do Setter para a Transformação Global do Nó
        void Model::setNodeGlobalTransform(const std::string &nodeName, const glm::mat4 &transform)
        {
            // Armazena no membro privado m_nodeGlobalTransforms
            m_nodeGlobalTransforms[nodeName] = transform;
        }

        // IMPLEMENTAÇÃO DE getSkeletonDebugLines (Nova assinatura e lógica)
        std::vector<glm::vec3> Model::getSkeletonDebugLines() const
        {
            // A lógica assume que 'finalTransformation' no Skeleton foi atualizada pelo AnimationSystem.
            std::vector<glm::vec3> lines;
            if (!m_skeleton || m_skeleton->bones.empty())
            {
                return lines;
            }

            // O cálculo é: MatrizGlobal = MatrizFinal * (IBM^-1)

            for (const auto &bone : m_skeleton->bones)
            {
                // Se o bone não tem pai, é o root. Pulamos para desenhar apenas as conexões.
                if (bone.parentId == -1 || bone.parentId >= m_skeleton->bones.size())
                {
                    continue;
                }

                const Bone &parentBone = m_skeleton->bones[bone.parentId];

                // 1. Matriz Global do Bone
                // Para debug, usamos a inversa da IBM, que é a matriz de pose de bind (BindPoseMatrix).
                // MatrizGlobal = MatrizFinal * Inv(IBM)
                glm::mat4 boneBindInverse = glm::inverse(bone.inverseBindMatrix);
                glm::mat4 boneGlobal = bone.finalTransformation * boneBindInverse;

                // 2. Matriz Global do Pai
                glm::mat4 parentBindInverse = glm::inverse(parentBone.inverseBindMatrix);
                glm::mat4 parentGlobal = parentBone.finalTransformation * parentBindInverse;

                // 3. Posições (O vetor de translação na coluna 4, índice 3)
                glm::vec3 bonePosition = glm::vec3(boneGlobal[3]);
                glm::vec3 parentPosition = glm::vec3(parentGlobal[3]);

                // 4. Adicionar a linha (Ponto A e Ponto B)
                lines.push_back(parentPosition);
                lines.push_back(bonePosition);
            }

            return lines;
        }

    } // namespace Asset
} // namespace Engine