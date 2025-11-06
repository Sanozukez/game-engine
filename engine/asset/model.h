// src/engine/asset/model.h

#pragma once

#include "skeleton.h"  // <-- ADICIONAR
#include "animation.h" // <-- ADICIONAR
#include <vector>
#include <string>
#include <memory>
#include <glm/glm.hpp>
#include "./../../engine/render/material.h"
#include "./../../engine/math/quat.h"

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
            // NOVO: construtor com nodeTransform
            // src/engine/asset/model.h
            Mesh(std::vector<Vertex> &&vertices,
                 std::vector<uint32_t> &&indices,
                 std::unique_ptr<Render::Material> material,
                 const glm::mat4 &nodeTransform);

            // COMPATIBILIDADE: construtor antigo (3 args) → node = Identity
            Mesh(std::vector<Vertex> &&vertices,
                 std::vector<uint32_t> &&indices,
                 std::unique_ptr<Render::Material> material);

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

            // NOVO: transform do nó que contém a malha no glTF
            glm::mat4 m_nodeTransform{1.0f};

            void setupMesh();
        };

        class Model
        {
        public:
            Model();
            ~Model();

            void addMesh(std::unique_ptr<Mesh> mesh);
            void draw(Engine::Render::Shader &shader, const std::vector<glm::mat4> *boneTransforms = nullptr);
            const std::vector<std::unique_ptr<Mesh>> &getMeshes() const { return m_meshes; }

            std::unique_ptr<Model> clone() const;

            const Skeleton *getSkeleton() const { return m_skeleton.get(); }                   // <-- ATUALIZADO
            Skeleton *getSkeleton() { return m_skeleton.get(); }                               // <-- ATUALIZADO
            void setSkeleton(std::unique_ptr<Skeleton> skel) { m_skeleton = std::move(skel); } // <-- ATUALIZADO

            // NOVO: Métodos de Animação (Animation)
            void addAnimation(uint32_t nameHash, std::unique_ptr<Animation> anim); // <-- ADICIONAR
            const Animation *getAnimation(uint32_t nameHash) const;                // <-- ADICIONAR

            const std::vector<std::string> getNodeChildren(const std::string &nodeName) const;
            const glm::mat4 getNodeLocalTransform(const std::string &nodeName) const;

            int getBoneIndexByName(const std::string &boneName) const
            {
                if (m_skeleton)
                {
                    auto it = m_skeleton->boneNameMap.find(boneName);
                    if (it != m_skeleton->boneNameMap.end())
                        return it->second;
                }
                return -1;
            }

            void addNode(const Node &node);
            void setNodeGlobalTransform(const std::string &nodeName, const glm::mat4 &transform);
            std::vector<glm::vec3> getSkeletonDebugLines() const;

            void setSkeletonBindTransform(const glm::mat4 &m) { m_skeletonBindTransform = m; }
            const glm::mat4 &getSkeletonBindTransform() const { return m_skeletonBindTransform; }

            /**
             * @brief Verifica se o modelo tem dados de esqueleto.
             */
            bool hasSkeleton() const { return m_skeleton != nullptr; }

        private:
            std::vector<std::unique_ptr<Mesh>> m_meshes;

            std::unordered_map<std::string, glm::mat4> m_nodeGlobalTransforms;
            std::unordered_map<std::string, Node> m_nodeHierarchy;

            glm::mat4 m_skeletonBindTransform{1.0f};

            // Membros para Animação e Esqueleto
            std::unique_ptr<Skeleton> m_skeleton;                                  // <-- CORREÇÃO: Aplicar m_ prefix
            std::unordered_map<uint32_t, std::unique_ptr<Animation>> m_animations; // <-- CORREÇÃO: Tipo e m_ prefix
        };

    }
} // namespaces
