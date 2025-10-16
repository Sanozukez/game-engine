// engine/asset/model.h
#pragma once

#include <vector>
#include <string>
#include <memory> // Para std::unique_ptr

#include <glm/glm.hpp>

#include "./../../engine/render/material.h"
#include "./../../engine/math/quat.h"

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

        struct BoneInfo
        {
            int id;           // ID que será usado para indexar o array de matrizes no Shader (0 a MAX_BONES-1)
            glm::mat4 offset; // Matriz Inverse Bind Pose (Bone Space -> Mesh Space)
        };

        // 1. KeyFrame: Ponto de dado no tempo
        template <typename T>
        struct KeyFrame
        {
            float time;
            T value;
        };

        // 2. BoneChannel: Contém os trilhos (tracks) de animação para um osso
        struct BoneChannel
        {
            std::vector<KeyFrame<glm::vec3>> positionKeys;
            std::vector<KeyFrame<Engine::Math::Quat>> rotationKeys; // Usar Quat para rotação suave
            std::vector<KeyFrame<glm::vec3>> scaleKeys;
            std::string boneName;
            // Futuro: InterpolationType (LINEAR, STEP, CUBICSPLINE)
        };

        // 3. AnimationClip: Contém todos os canais de um clipe (ex: "Run", "Idle")
        struct AnimationClip
        {
            std::unordered_map<std::string, BoneChannel> boneChannels;
            float duration = 0.0f; // Duração total do clipe em segundos
        };

        struct Node
        {
            std::string name;
            glm::mat4 localTransform;
            std::vector<std::string> childrenNames; // Nomes dos nodos filhos
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

            void draw(Engine::Render::Shader &shader, const std::vector<glm::mat4> *boneTransforms = nullptr);

            const std::vector<std::unique_ptr<Mesh>> &getMeshes() const { return m_meshes; }

            // NOVO: Declaração da função clone para permitir a instanciação
            // Retorna uma cópia profunda (deep copy) do Model como um ponteiro único.
            std::unique_ptr<Model> clone() const;

            // -------------------------------------------------------------
            // NOVO: Métodos de Interface para o AnimationUtils (DIP)
            // -------------------------------------------------------------

            // Expor o mapa de ossos
            const std::unordered_map<std::string, BoneInfo> &getBoneInfoMap() const { return m_boneInfoMap; }

            // Expor o nome do nodo raiz do esqueleto
            const std::string &getSkeletonRootName() const { return m_skeletonRootNodeName; }

            // Expor a contagem total de ossos
            int getBoneCount() const { return m_boneCounter; }

            // -------------------------------------------------------------
            // NOVO: Métodos de Interface para o GLTFLoader (Para Preencher os Dados)
            // -------------------------------------------------------------

            // Define o nome do nodo raiz
            void setSkeletonRootName(const std::string &rootName) { m_skeletonRootNodeName = rootName; }

            // Adiciona um osso ao mapa
            void addBone(const std::string &name, const glm::mat4 &offset);

            // NOVO: Expor a estrutura de dados de Nodo (Para a Travessia)
            // Usamos um placeholder genérico (string) por enquanto.
            // O Model precisa saber os filhos para a recursão:
            const std::vector<std::string> getNodeChildren(const std::string &nodeName) const;

            // NOVO: Expor a Transformação Local (Pose) de um nodo
            const glm::mat4 getNodeLocalTransform(const std::string &nodeName) const;

            // NOVO: Mapear Nome do Osso para o ID do Shader (Integração)
            int getBoneIndexByName(const std::string &boneName) const;

            // -------------------------------------------------------------
            // NOVO: Interface para o AnimationLoader
            // -------------------------------------------------------------
            void addAnimationClip(uint32_t nameHash, AnimationClip clip) { m_animationClips[nameHash] = std::move(clip); }
            const AnimationClip *getAnimationClip(uint32_t nameHash) const;

            // -------------------------------------------------------------
            // NOVO: Interface de Escrita (para o Loader)
            // -------------------------------------------------------------
            void addNode(const Node &node);

        private:
            std::vector<std::unique_ptr<Mesh>> m_meshes;

            // --- NOVO: Membros do Skeleton ---
            std::unordered_map<std::string, BoneInfo> m_boneInfoMap; // Mapeia Nome do Osso -> Dados
            std::string m_skeletonRootNodeName;                      // Nome do nodo raiz do esqueleto (Ex: "Armature")
            int m_boneCounter = 0;                                   // Contador de ossos (para IDs únicos)

            // --- NOVO MEMBRO: Armazena todos os clipes de animação lidos ---
            // A chave é o Hash ID do nome do clipe (ex: hash("Run"))
            std::unordered_map<uint32_t, AnimationClip> m_animationClips;

            // --- NOVO MEMBRO: Armazena a hierarquia completa de nodos ---
            // A chave é o nome do nodo
            std::unordered_map<std::string, Node> m_nodeHierarchy;
        };

    } // namespace Asset
} // namespace Engine