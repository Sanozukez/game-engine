// engine/asset/model.h

#pragma once

#include "skeleton.h"
#include "animation.h" // (Agora define AnimationAsset)
#include <vector>
#include <string>
#include <memory>
#include <glm/glm.hpp>
#include "./../../client/render/material.h"
#include "./../../engine/math/quat.h"
#include "./../../engine/core/log.h"
#include <format>

namespace Engine
{
    namespace Render
    {
        class Shader;
    }
}

namespace Engine
{
    namespace Asset
    {
        // ... (struct Vertex e struct Node permanecem iguais) ...
        struct Vertex
        {
            glm::vec3 Position;
            glm::vec3 Normal;
            glm::vec2 TexCoords;
            glm::vec3 Tangent;
            glm::ivec4 BoneIDs = glm::ivec4(0);
            glm::vec4 Weights = glm::vec4(0.0f);
        };

        struct Node
        {
            std::string name;
            glm::mat4 localTransform;
            std::vector<std::string> childrenNames;
        };


        class Mesh
        {
        public:
            // --- CORREÇÃO: Construtores da Mesh refatorados (como discutimos) ---
            Mesh(std::vector<Vertex> &&vertices,
                 std::vector<uint32_t> &&indices,
                 std::unique_ptr<Render::Material> material,
                 const glm::mat4 &nodeTransform = glm::mat4(1.0f)); // <-- Valor padrão
            // (O construtor de 3 argumentos foi removido)
            // --- FIM DA CORREÇÃO ---

            ~Mesh();
            Mesh(const Mesh &other);

            void draw(Engine::Render::Shader &shader);

            size_t getVertexCount() const { return m_vertices.size(); }
            size_t getIndexCount() const { return m_indices.size(); }
            const Render::Material *getMaterial() const { return m_material.get(); }
            const std::vector<Vertex> &getVertices() const { return m_vertices; }
            const std::vector<uint32_t> &getIndices() const { return m_indices; }
            const glm::mat4 &getNodeTransform() const { return m_nodeTransform; }

        private:
            std::vector<Vertex> m_vertices;
            std::vector<uint32_t> m_indices;
            std::unique_ptr<Render::Material> m_material;
            unsigned int m_VAO = 0, m_VBO = 0, m_EBO = 0;
            glm::mat4 m_nodeTransform{1.0f};
            void setupMesh();
        };

        class Model
        {
        public:
            Model();
            ~Model();

            // (Declarações de Cópia/Movimento que você já adicionou)
            Model(const Model &other);
            Model(Model &&other) noexcept;
            Model &operator=(const Model &other);
            Model &operator=(Model &&other) noexcept;

            void addMesh(std::unique_ptr<Mesh> mesh);
            void draw(Engine::Render::Shader &shader, const std::vector<glm::mat4> *boneTransforms = nullptr);
            const std::vector<std::unique_ptr<Mesh>> &getMeshes() const { return m_meshes; }

            std::unique_ptr<Model> clone() const;

            const Skeleton *getSkeleton() const { return m_skeleton.get(); }
            Skeleton *getSkeleton() { return m_skeleton.get(); }
            void setSkeleton(std::unique_ptr<Skeleton> skel) { m_skeleton = std::move(skel); }

            // (Já estava correto com AnimationAsset)
            void addAnimation(uint32_t nameHash, std::unique_ptr<AnimationAsset> anim);
            
            // --- CORREÇÃO: Adicionado ';' em falta ---
            const AnimationAsset *getAnimation(uint32_t nameHash) const;
            
            // NOVO v101: Buscar animação por nome de string (não hash)
            const AnimationAsset* getAnimationByName(const std::string& name) const;
            uint32_t getAnimationIndex(const std::string& name) const;
            size_t getAnimationCount() const { return m_animations.size(); }
            // --- FIM DA CORREÇÃO ---

            const std::vector<std::string> getNodeChildren(const std::string &nodeName) const;
            const glm::mat4 getNodeLocalTransform(const std::string &nodeName) const;

            // --- CORREÇÃO: Removida implementação em 'const;' ---
            int getBoneIndexByName(const std::string &boneName) const;
            // (A implementação vai para o .cpp)
            // --- FIM DA CORREÇÃO ---

            void addNode(const Node &node);
            void setNodeGlobalTransform(const std::string &nodeName, const glm::mat4 &transform);
            std::vector<glm::vec3> getSkeletonDebugLines() const;

            void setSkeletonBindTransform(const glm::mat4 &m) { m_skeletonBindTransform = m; }
            const glm::mat4 &getSkeletonBindTransform() const { return m_skeletonBindTransform; }

            bool hasSkeleton() const { return m_skeleton != nullptr; }

            const glm::mat4& getNodeGlobalTransform(const std::string& nodeName) const;

        private:
            std::vector<std::unique_ptr<Mesh>> m_meshes;
            std::unordered_map<std::string, glm::mat4> m_nodeGlobalTransforms;
            std::unordered_map<std::string, Node> m_nodeHierarchy;
            glm::mat4 m_skeletonBindTransform{1.0f};

            std::unique_ptr<Skeleton> m_skeleton; 
            std::unordered_map<uint32_t, std::unique_ptr<AnimationAsset>> m_animations;
        };

    } // namespace Asset
} // namespace Engine