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

using namespace Engine::ECS::Component;

namespace Engine
{
    namespace Asset
    {
        // --- Mesh Class ---
        Mesh::Mesh(std::vector<Vertex>&& vertices,
           std::vector<uint32_t>&& indices,
           std::unique_ptr<Render::Material> material,
           const glm::mat4& nodeTransform)
: m_vertices(std::move(vertices))
, m_indices(std::move(indices))
, m_material(std::move(material))
, m_nodeTransform(nodeTransform)
{
    setupMesh();
}

// Construtor “antigo” (compat) → node = Identity
Mesh::Mesh(std::vector<Vertex>&& vertices,
           std::vector<uint32_t>&& indices,
           std::unique_ptr<Render::Material> material)
: m_vertices(std::move(vertices))
, m_indices(std::move(indices))
, m_material(std::move(material))
, m_nodeTransform(1.0f)
{
    setupMesh();
}

Mesh::Mesh(const Mesh& other)
: m_vertices(other.m_vertices)
, m_indices(other.m_indices)
, m_material(other.m_material ? other.m_material->clone() : nullptr)
, m_nodeTransform(other.m_nodeTransform)
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

        void Model::addBone(const std::string &name, const glm::mat4 &offset)
        {
            // O Model armazena os dados, o GLTFLoader é quem preenche.
            // Garante que o osso não seja adicionado duas vezes.
            if (m_boneInfoMap.find(name) == m_boneInfoMap.end())
            {
                BoneInfo info;
                info.id = m_boneCounter;
                info.offset = offset;
                m_boneInfoMap[name] = info;
                m_boneCounter++;
            }
        }

        void Model::addBone(const std::string &name, const glm::mat4 &offset, int forcedId)
        {
            // Nova versão: força o ID igual ao índice do joint do GLTF
            if (m_boneInfoMap.find(name) == m_boneInfoMap.end())
            {
                BoneInfo info;
                info.id = forcedId;
                info.offset = offset;
                m_boneInfoMap[name] = info;

                // Garante que o contador nunca fique menor que o maior ID já adicionado
                m_boneCounter = std::max(m_boneCounter, forcedId + 1);
            }
        }

        void Model::addNode(const Node &node)
        {
            m_nodeHierarchy[node.name] = node;
        }

        // NOVO : Shell de getBoneIndexByName
        int Model::getBoneIndexByName(const std::string &boneName) const
        {
            auto it = m_boneInfoMap.find(boneName);
            if (it != m_boneInfoMap.end())
            {
                return it->second.id;
            }
            return -1;
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

        // A implementação de getSkeletonRootName() e getBoneInfoMap() são inline (no .h),
        // então elas não precisam ser definidas aqui.

        const AnimationClip *Model::getAnimationClip(uint32_t nameHash) const
        {
            if (m_animationClips.count(nameHash))
            {
                return &m_animationClips.at(nameHash);
            }
            return nullptr;
        }

        void Model::draw(Engine::Render::Shader &shader, const std::vector<glm::mat4> *boneTransforms)
        {
            // NOVO: Se boneTransforms estiver presente, enviar para o shader.
            if (boneTransforms != nullptr)
            {
                // Certifique-se de que o MAX_BONES está visível. Usamos a constante do Componente.
                for (size_t i = 0; i < boneTransforms->size() && i < Animation::MAX_BONES; ++i)
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

        // NOVO: Implementação da função que extrai a geometria das linhas do esqueleto.
        // A posição de cada bone é a translação final (matriz[3]).
        std::vector<glm::vec3> Model::getSkeletonDebugLines(const std::vector<glm::mat4> &finalBoneTransforms) const
        {
            std::vector<glm::vec3> lines;

            // ATENÇÃO: Itera sobre a HIERARQUIA, não sobre o m_nodeGlobalTransforms diretamente.
            for (const auto &pair : m_nodeHierarchy)
            {
                const Node &node = pair.second;

                // 1. Tenta obter a matriz Global do nó (matriz de posição animada)
                if (m_nodeGlobalTransforms.count(node.name))
                { // Verifica se este nó tem uma Global Transform animada

                    glm::mat4 currentBoneGlobal = m_nodeGlobalTransforms.at(node.name);
                    glm::vec3 currentPos = glm::vec3(currentBoneGlobal[3]);

                    // 2. Itera sobre os filhos
                    for (const auto &childName : node.childrenNames)
                    {

                        // Pega a posição do filho APENAS se ele também tem uma matriz Global armazenada.
                        if (m_nodeGlobalTransforms.count(childName))
                        {

                            glm::mat4 childBoneGlobal = m_nodeGlobalTransforms.at(childName);
                            glm::vec3 childPos = glm::vec3(childBoneGlobal[3]);

                            // Adiciona as coordenadas (Ponto A e Ponto B)
                            lines.push_back(currentPos); // Ponto A (Pai)
                            lines.push_back(childPos);   // Ponto B (Filho)
                        }
                    }
                }
            }
            return lines;
        }

    } // namespace Asset
} // namespace Engine