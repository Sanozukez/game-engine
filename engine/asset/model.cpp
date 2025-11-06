// engine/asset/model.cpp
#include "model.h"
#include "./../core/log.h"
#include "./../render/shader.h"
#include "../render/opengl_types.h"
#include "../ecs/components/component_signature.h"
#include <cstddef>
#include <format>
#include <vector>
#include <functional> // (Para getSkeletonDebugLines, se usássemos std::function)
#include "skeleton.h"
#include "animation.h" // (Agora define AnimationAsset)

// (Não precisamos disto se usarmos os nomes completos,
// mas é necessário para a constante MAX_BONES se a mantivéssemos)
// using namespace Engine::ECS::Component;

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
            glEnableVertexAttribArray(0);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, Position));
            glEnableVertexAttribArray(1);
            glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, Normal));
            glEnableVertexAttribArray(2);
            glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, TexCoords));
            glEnableVertexAttribArray(3);
            glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, Tangent));
            glEnableVertexAttribArray(4);
            glVertexAttribIPointer(4, 4, GL_INT, sizeof(Vertex), (void *)offsetof(Vertex, BoneIDs));
            glEnableVertexAttribArray(5);
            glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, Weights));
            glBindVertexArray(0);
        }

        void Engine::Asset::Mesh::draw(Engine::Render::Shader &shader)
        {
            shader.setMat4("uNode", m_nodeTransform);
            if (m_material)
            {
                m_material->activate(shader);
            }
            glBindVertexArray(m_VAO);
            glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(m_indices.size()), GL_UNSIGNED_INT, 0);
            glBindVertexArray(0);
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

        // (Implementações de Cópia/Movimento que você já tem)
        Model::Model(const Model &other)
            : m_nodeGlobalTransforms(other.m_nodeGlobalTransforms),
              m_nodeHierarchy(other.m_nodeHierarchy),
              m_skeletonBindTransform(other.m_skeletonBindTransform),
              m_skeleton(nullptr),
              m_animations()
        {
            Engine::Core::Log::Critical("[DEBUG_MODEL] CONSTRUTOR DE CÓPIA DO MODELO FOI CHAMADO! Isto cria um Modelo sem esqueleto!");
            m_meshes.clear();
            for (const auto &mesh : other.m_meshes)
            {
                if (mesh)
                {
                    m_meshes.push_back(std::make_unique<Mesh>(*mesh));
                }
            }
        }

        Model::Model(Model &&other) noexcept
            : m_meshes(std::move(other.m_meshes)),
              m_nodeGlobalTransforms(std::move(other.m_nodeGlobalTransforms)),
              m_nodeHierarchy(std::move(other.m_nodeHierarchy)),
              m_skeletonBindTransform(std::move(other.m_skeletonBindTransform)),
              m_skeleton(std::move(other.m_skeleton)),
              m_animations(std::move(other.m_animations))
        {
            Engine::Core::Log::Trace("[DEBUG_MODEL] Construtor de Movimento do Modelo foi chamado.");
        }

        Model &Model::operator=(const Model &other)
        {
            Engine::Core::Log::Critical("[DEBUG_MODEL] ATRIBUIÇÃO DE CÓPIA DO MODELO FOI CHAMADA! Isto cria um Modelo sem esqueleto!");
            if (this == &other)
                return *this;
            m_nodeGlobalTransforms = other.m_nodeGlobalTransforms;
            m_nodeHierarchy = other.m_nodeHierarchy;
            m_skeletonBindTransform = other.m_skeletonBindTransform;
            m_skeleton = nullptr;
            m_animations.clear();
            m_meshes.clear();
            for (const auto &mesh : other.m_meshes)
            {
                if (mesh)
                {
                    m_meshes.push_back(std::make_unique<Mesh>(*mesh));
                }
            }
            return *this;
        }

        Model &Model::operator=(Model &&other) noexcept
        {
            Engine::Core::Log::Trace("[DEBUG_MODEL] Atribuição de Movimento do Modelo foi chamada.");
            if (this == &other)
                return *this;
            m_meshes = std::move(other.m_meshes);
            m_nodeGlobalTransforms = std::move(other.m_nodeGlobalTransforms);
            m_nodeHierarchy = std::move(other.m_nodeHierarchy);
            m_skeletonBindTransform = std::move(other.m_skeletonBindTransform);
            m_skeleton = std::move(other.m_skeleton);
            m_animations = std::move(other.m_animations);
            return *this;
        }

        void Model::addMesh(std::unique_ptr<Mesh> mesh)
        {
            if (mesh)
            {
                m_meshes.push_back(std::move(mesh));
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

        void Model::addAnimation(uint32_t nameHash, std::unique_ptr<AnimationAsset> anim)
        {
            m_animations[nameHash] = std::move(anim);
        }

        const AnimationAsset *Model::getAnimation(uint32_t nameHash) const
        {
            auto it = m_animations.find(nameHash);
            return (it != m_animations.end()) ? it->second.get() : nullptr;
        }

        // --- CORREÇÃO: Implementação movida do .h para cá ---
        int Model::getBoneIndexByName(const std::string &boneName) const
        {
            if (m_skeleton)
            {
                auto it = m_skeleton->boneNameMap.find(boneName);
                if (it != m_skeleton->boneNameMap.end())
                    return it->second;
            }
            return -1;
        }
        // --- FIM DA CORREÇÃO ---

        void Model::draw(Engine::Render::Shader &shader, const std::vector<glm::mat4> *boneTransforms)
        {
            if (boneTransforms != nullptr)
            {
                // --- CORREÇÃO (SRP): Usa Skeleton::MAX_BONES em vez de AnimationComponent ---
                for (size_t i = 0; i < boneTransforms->size() && i < Skeleton::MAX_BONES; ++i)
                {
                    std::string uniformName = "uBoneTransforms[" + std::to_string(i) + "]";
                    shader.setMat4(uniformName, boneTransforms->at(i));
                }
            }
            for (const auto &mesh_ptr : m_meshes)
            {
                mesh_ptr->draw(shader);
            }
        }

        // Implementação da função clone() para Model
        std::unique_ptr<Model> Model::clone() const
        {
            Engine::Core::Log::Critical("[DEBUG_MODEL] Model::clone() foi chamado! Isto está a criar uma cópia do modelo SEM o esqueleto!");

            auto clonedModel = std::make_unique<Model>();
            // 2. Copia Profunda (Deep Copy) das Meshes
            // NOTA: Para que este loop funcione, a classe Mesh deve ter um método clone()!
            for (const auto &mesh : m_meshes)
            {
                if (mesh)
                {
                    clonedModel->m_meshes.push_back(std::make_unique<Mesh>(*mesh));
                    // ^^^^^ Assumindo construtor de cópia Mesh::Mesh(const Mesh&), que fará setupMesh().
                }
            }

            // 3. (Opcional) Copia outros dados do Model (se existirem)
            Engine::Core::Log::Trace(std::format("Model: Clonado com sucesso! {} meshes copiadas.", clonedModel->m_meshes.size()));
            // (Isto não copia o esqueleto nem as animações)
            return clonedModel;
        }

        // Implementação do Setter para a Transformação Global do Nó
        void Model::setNodeGlobalTransform(const std::string &nodeName, const glm::mat4 &transform)
        {
            // Armazena no membro privado m_nodeGlobalTransforms
            m_nodeGlobalTransforms[nodeName] = transform;
        }

        // IMPLEMENTAÇÃO DE getSkeletonDebugLines
        std::vector<glm::vec3> Model::getSkeletonDebugLines() const
        {

            std::vector<glm::vec3> lines;
            if (!m_skeleton || m_skeleton->bones.empty())
            {
                return lines;
            }

            int lineCount = 0;
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
                glm::vec3 bonePosition = glm::vec3(bone.debug_GlobalTransform[3]);
                glm::vec3 parentPosition = glm::vec3(parentBone.debug_GlobalTransform[3]);
                // 2. Adicionar a linha (Ponto A e Ponto B)
                lines.push_back(parentPosition);
                lines.push_back(bonePosition);
                lineCount++;
            }
            // --- NOVO LOG DE DEBUG ---
            Engine::Core::Log::Info(std::format("[DEBUG_MODEL] getSkeletonDebugLines: Geradas {} linhas de debug ({} vértices).", lineCount, lines.size()));
            return lines;
        }

    } // namespace Asset
} // namespace Engine